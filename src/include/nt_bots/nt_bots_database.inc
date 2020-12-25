#if defined(_NT_BOTS_DATABASE_INC_)
#endinput
#endif
#define _NT_BOTS_DATABASE_INC_

static Database _db = null;

static char current_map_jobs_db_table_name[PLATFORM_MAX_PATH];
static char current_map_solutions_db_table_name[PLATFORM_MAX_PATH];

void ConnectToDatabase()
{
	if (_db == null) {
		Database.Connect(GotDatabase, "storage-local");
	}
}

public void GotDatabase(Database db, const char[] error, any data)
{
	if (db == null) {
		SetFailState("Database connection error: %s", error);
	}
	_db = db;
}

static char cb_sql[256];

public void Cb_FireAndForget(Database db, DBResultSet results, const char[] error, any data)
{
}

public void Cb_GetSolution(Database db, DBResultSet results, const char[] error, DataPack data)
{
	data.Reset();
	
	int userid = data.ReadCell();
	int bot_client = GetClientOfUserId(userid);
	// Make sure bot didn't disconnect before we got this solution
	if (bot_client == 0) {
		delete data;
		return;
	}
	
	float pos_from[3];
	float pos_to[3];
	pos_from[0] = data.ReadFloat();
	pos_from[1] = data.ReadFloat();
	pos_from[2] = data.ReadFloat();
	pos_to[0] = data.ReadFloat();
	pos_to[1] = data.ReadFloat();
	pos_to[2] = data.ReadFloat();
	
	if (results == null) {
		delete data;
		SetFailState("Cb_GetSolution error: %s", error);
	}
	
	//PrintToServer("We've got results (RowCount: %d, FieldCount: %d, more rows: %s)",
	//	results.RowCount,
	//	results.FieldCount,
	//	(results.MoreRows ? "yes" : "no"));
	
	if (results.RowCount != 0) {
		ClearPlan(bot_client);
		float xyz[3];
		bool got_any_result = false;
		while (results.FetchRow()) {
			xyz[0] = results.FetchFloat(8);
			xyz[1] = results.FetchFloat(9);
			xyz[2] = results.FetchFloat(10);
			PlanMove(bot_client, xyz);
			got_any_result = true;
			
			// In case we got multiple sets of the same path for whatever reason
			if (VectorsEqual(pos_to, xyz)) {
				break;
			}
		}
		
		if (got_any_result) {
			new const String:delete_schema[] = "DELETE FROM %s WHERE from_area_x = %.1f AND from_area_y = %.1f AND from_area_z = %.1f AND \
to_area_x = %.1f AND to_area_y = %.1f AND to_area_z = %.1f;";
			
			Format(cb_sql, sizeof(cb_sql), delete_schema,
				current_map_solutions_db_table_name,
				pos_from[0], pos_from[1], pos_from[2],
				pos_to[0], pos_to[1], pos_to[2]);
			
			_db.Query(Cb_FireAndForget, cb_sql);
		}
		else {
			delete data;
			SetFailState("Solution cb fired, but no result!");
		}
	}
	else {
		//PrintToServer("No result; requesting");
		
		// Sqlite db has constraint for duplicate rows, so we can just (re) insert
		new const String:insert_schema[] = "INSERT INTO %s (epoch, from_area_x, from_area_y, from_area_z, \
to_area_x, to_area_y, to_area_z) VALUES (%d, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f);";
		
		Format(cb_sql, sizeof(cb_sql), insert_schema,
			current_map_jobs_db_table_name,
			GetTime(),
			pos_from[0], pos_from[1], pos_from[2],
			pos_to[0], pos_to[1], pos_to[2]);
		
		_db.Query(Cb_FireAndForget, cb_sql);
		
		CreateTimer(1.0, Timer_DbTryAgain, data, TIMER_FLAG_NO_MAPCHANGE);
		return;
	}
	
	delete data;
}

public Action Timer_DbTryAgain(Handle timer, DataPack data)
{
	data.Reset();
	int bot_client = GetClientOfUserId(data.ReadCell());
	float pos_from[3];
	float pos_to[3];
	pos_from[0] = data.ReadFloat();
	pos_from[1] = data.ReadFloat();
	pos_from[2] = data.ReadFloat();
	pos_to[0] = data.ReadFloat();
	pos_to[1] = data.ReadFloat();
	pos_to[2] = data.ReadFloat();
	
	if (bot_client == 0) {
		delete data;
	}
	else {
		decl String:sql[256];
		
		new const String:select_schema[] = "SELECT * FROM %s WHERE from_area_x = %.1f AND \
	from_area_y = %.1f AND from_area_z = %.1f AND to_area_x = %.1f AND to_area_y = %.1f AND \
	to_area_z = %.1f ORDER BY epoch DESC;";
		Format(sql, sizeof(sql), select_schema, current_map_solutions_db_table_name,
			pos_from[0], pos_from[1], pos_from[2],
			pos_to[0], pos_to[1], pos_to[2]);
		
		_db.Query(Cb_GetSolution, sql, data);
	}
	
	return Plugin_Stop;
}

void TrySolution(int bot_client, const float[3] pos_from, const float[3] pos_to)
{
	DataPack data = new DataPack();
	data.WriteCell(GetClientUserId(bot_client));
	data.WriteFloat(pos_from[0]);
	data.WriteFloat(pos_from[1]);
	data.WriteFloat(pos_from[2]);
	data.WriteFloat(pos_to[0]);
	data.WriteFloat(pos_to[1]);
	data.WriteFloat(pos_to[2]);
	
	Timer_DbTryAgain(INVALID_HANDLE, data);
}

void RequestPath(int client, const float[3] from, const float[3] to)
{
	//PrintToServer("RequestPath: %d, (%f %f %f), (%f %f %f)",
	//	client,
	//	from[0], from[1], from[2],
	//	to[0], to[1], to[2]);
	
	float from_rounded[3];
	float to_rounded[3];
	
	from_rounded = from;
	to_rounded = to;

#define DB_PATH_ROUNDING BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE_MIN
#if(0)
#if DB_PATH_ROUNDING != 64
#warning Ensure db compatibility with DB_PATH_ROUNDING
#endif
#endif
	
	RoundVector(from_rounded, DB_PATH_ROUNDING);
	RoundVector(to_rounded, DB_PATH_ROUNDING);
	
	TrySolution(client, from_rounded, to_rounded);
}

void UpdateDbTableNamesForThisMap()
{
	decl String:current_map_name[PLATFORM_MAX_PATH];
	
	if (GetCurrentMap(current_map_name, sizeof(current_map_name)) < 1) {
		SetFailState("Failed to get current map name");
	}
	
	decl String:map_path_buffer[PLATFORM_MAX_PATH];
	Format(map_path_buffer, sizeof(map_path_buffer), "maps/%s.bsp", current_map_name);
	int current_map_size = FileSize(map_path_buffer);
	if (current_map_size == -1) {
		SetFailState("Failed to get current map size for %s from path: \"%s\"",
			current_map_name, map_path_buffer);
	}
	
	if (current_map_name[0] == '\0') {
		SetFailState("Failed to fetch current map name");
	}
	else if (current_map_size <= 0) {
		SetFailState("Failed to fetch current map size");
	}
	
	new const String:db_jobs_table_id[] = "nabejobs";
	if (Format(current_map_jobs_db_table_name, sizeof(current_map_jobs_db_table_name), "%s_%d_%s",
		db_jobs_table_id, current_map_size, current_map_name) < 1)
	{
		SetFailState("Failed to format jobs db table name");
	}
	
	new const String:db_solutions_table_id[] = "nabesols";
	if (Format(current_map_solutions_db_table_name, sizeof(current_map_solutions_db_table_name), "%s_%d_%s",
		db_solutions_table_id, current_map_size, current_map_name) < 1)
	{
		SetFailState("Failed to format solutions db table name");
	}
}
