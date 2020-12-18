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
		decl String:error[256];
		_db = SQLite_UseDatabase("sourcemod-local", error, sizeof(error));
		if (_db == null) {
			SetFailState("SQLite connection error: %s", error);
		}
	}
}

public void Cb_GetSolution(Database db, DBResultSet results, const char[] error, DataPack data)
{
	data.Reset();
	//int bot_client = data.ReadCell();
	float pos_from[3];
	float pos_to[3];
	pos_from[0] = data.ReadFloat();
	pos_from[1] = data.ReadFloat();
	pos_from[2] = data.ReadFloat();
	pos_to[0] = data.ReadFloat();
	pos_to[1] = data.ReadFloat();
	pos_to[2] = data.ReadFloat();
	delete data;
	
	if (results == null) {
		SetFailState("Cb_GetSolution error: %s", error);
	}
	else {
		//PrintToServer("We've got results (RowCount: %d, FieldCount: %d, more rows: %s)",
		//	results.RowCount,
		//	results.FieldCount,
		//	(results.MoreRows ? "yes" : "no"));
		
		decl String:sql[256];
		
		if (results.RowCount != 0) {
			//PrintToServer("Had actual result");
			
			ClearPlan();
			float xyz[3];
			bool got_any_result = false;
			while (results.FetchRow()) {
				xyz[0] = results.FetchFloat(7);
				xyz[1] = results.FetchFloat(8);
				xyz[2] = results.FetchFloat(9);
				PlanMove(xyz);
				got_any_result = true;
			}
			
			if (got_any_result) {
				new const String:delete_schema[] = "DELETE FROM %s WHERE from_area_x = %.1f AND from_area_y = %.1f AND from_area_z = %.1f AND \
to_area_x = %.1f AND to_area_y = %.1f AND to_area_z = %.1f;";
				
				Format(sql, sizeof(sql), delete_schema,
					current_map_jobs_db_table_name,
					pos_from[0], pos_from[1], pos_from[2],
					pos_to[0], pos_to[1], pos_to[2]);
				
				SQL_LockDatabase(_db);
				SQL_FastQuery(_db, sql);
				SQL_UnlockDatabase(_db);
			}
		}
		else {
			//PrintToServer("No result; requesting");
			
			new const String:insert_schema[] = "INSERT INTO %s (from_area_x, from_area_y, from_area_z, \
to_area_x, to_area_y, to_area_z) VALUES (%.1f, %.1f, %.1f, %.1f, %.1f, %.1f);";
			
			Format(sql, sizeof(sql), insert_schema,
				current_map_jobs_db_table_name,
				pos_from[0], pos_from[1], pos_from[2],
				pos_to[0], pos_to[1], pos_to[2]);
			
			SQL_LockDatabase(_db);
			SQL_FastQuery(_db, sql);
			SQL_UnlockDatabase(_db);
			
			//if (!AddPendingSolution(area_from, area_to)) {
			//	SetFailState("Solution queue filled up! This shouldn't happen.");
			//}
		}
	}
}

void TrySolution(const float[3] pos_from, const float[3] pos_to)
{
	decl String:sql[256];
	
	new const String:select_schema[] = "SELECT * FROM %s WHERE from_area_x = %.1f AND \
from_area_y = %.1f AND from_area_z = %.1f AND to_area_x = %.1f AND to_area_y = %.1f AND \
to_area_z = %.1f;";
	Format(sql, sizeof(sql), select_schema, current_map_solutions_db_table_name,
		pos_from[0], pos_from[1], pos_from[2],
		pos_to[0], pos_to[1], pos_to[2]);
	
	//int bot_client = 0;
	//for (int client = 1; client <= MaxClients; ++client) {
	//	if (IsClientInGame(client) && IsFakeClient(client)) {
	//		bot_client = client;
	//		break;
	//	}
	//}
	//if (bot_client == 0) {
	//	SetFailState("Failed to find the bot!");
	//}
	
	DataPack data = new DataPack();
	//data.WriteCell(bot_client);
	data.WriteFloat(pos_from[0]);
	data.WriteFloat(pos_from[1]);
	data.WriteFloat(pos_from[2]);
	data.WriteFloat(pos_to[0]);
	data.WriteFloat(pos_to[1]);
	data.WriteFloat(pos_to[2]);
	_db.Query(Cb_GetSolution, sql, data);
}

void RequestPath(const float[3] from, const float[3] to)
{
	float from_rounded[3];
	float to_rounded[3];
	
	from_rounded = from;
	to_rounded = to;
	
	RoundVector(from_rounded, BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE);
	RoundVector(to_rounded, BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE);
	
	TrySolution(from_rounded, to_rounded);
}

static void UpdateDbTableNamesForThisMap()
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
