#if !defined(_NT_BOTS_BASE_INC_)
#define _NT_BOTS_BASE_INC_

int num_areas_in_the_nav_list;

stock float AreaDistance(const int id_a, const int id_b)
{
	NavArea a[1]; NavArea b[1];
	if (!GetAreaById(id_a, a)) { ThrowError("Failed to get id"); }
	if (!GetAreaById(id_b, b)) { ThrowError("Failed to get id"); }
	
	return GetVectorDistance(a[0].centroid, b[0].centroid);
}

stock void ListAreas()
{
	NavArea area;
	int num_iterated = 0;
	for (int i = TheNavAreaList.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		TheNavAreaList.GetArray(i, area);
		PrintToServer("= = = = = = = = = = = = = = = = = = = =\nFound NavArea at idx: %d (id value: %d)", i, area.id);
		float origin[3];
		area.GetCentroid(origin);
		PrintToServer("XY bounds: [ NW %f %f -  SE %f %f ], and origin: %f %f %f)",
			area.extents_nw_corner[0], area.extents_nw_corner[1],
			area.extents_se_corner[0], area.extents_se_corner[1],
			origin[0], origin[1], origin[2]);
		
		for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
			for (int j = 0; j < area.num_connected_areas[dir]; ++j) {
				if (j == 0) {
					PrintToServer("\n\t- NavArea %d has %d %sern connections total.",
						area.id, area.num_connected_areas[dir], nav_dir_names[dir]);
				}
				
				int connection = -1;
				switch (dir)
				{
					case DIR_NORTH: { connection = area.connected_areas_north[j]; }
					case DIR_EAST: { connection = area.connected_areas_east[j]; }
					case DIR_SOUTH: { connection = area.connected_areas_south[j]; }
					case DIR_WEST: { connection = area.connected_areas_west[j]; }
				}
				PrintToServer("\t-- NavArea %d has %sern connection: %d",
					area.id, nav_dir_names[dir], connection);
			}
		}
	}
}

stock int GetPathSolution(const int start_area_id, const int target_area_id, int[] solution_path_ids,
	const int max_solution_length = MAX_AREAS)
{
	NavArea start_area[1];
	NavArea target_area[1];
	
	if (!GetAreaById(start_area_id, start_area)) {
		SetFailState("!GetAreaById start_area_id");
	}
	if (!GetAreaById(target_area_id, target_area)) {
		SetFailState("!GetAreaById target_area_id");
	}
	
	if (start_area[0].id == target_area[0].id) {
		return 0;
	} else if (max_solution_length < 2) {
		ThrowError("Expected 2 to have space for at least 2 nodes");
		return 0;
	}
	
	//float goal_distance = target_area[0].DistanceTo(start_area[0].id);
	
	int bailout = 0;
	bool found_solution = false;
	
	int num_solution_ids = 0;
	
	while (!found_solution) {
		if (bailout++ >= MAX_AREAS) {
			SetFailState("Bail out");
		}
		
		num_solution_ids = 0;
		solution_path_ids[num_solution_ids++] = target_area[0].id;
		
		float prospective_distance = 0.0;
		
		int iter_id = target_area[0].id;
		
		while (!found_solution) {
			NavArea iter_area[1];
			if (!GetAreaById(iter_id, iter_area)) {
				SetFailState("!GetAreaById: %d", iter_id);
			}
			
			NavArea connected_area[1];
			if (!GetAreaById(iter_id, connected_area)) {
				SetFailState("!GetAreaById: %d", iter_id);
			}
			
			float distance = iter_area[0].DistanceTo(connected_area[0].id);
			prospective_distance += distance;
			
			for (NavDirType dir = NORTH; !found_solution && dir < NUM_DIRECTIONS; ++dir) {
				for (int j = 0; !found_solution && j < connected_area[0].num_connected_areas[dir]; ++j) {
					int connection = 0;
					switch (dir) {
						case NORTH: { connection = connected_area[0].connected_areas_north[j]; }
						case EAST: { connection = connected_area[0].connected_areas_east[j]; }
						case SOUTH: { connection = connected_area[0].connected_areas_south[j]; }
						case WEST: { connection = connected_area[0].connected_areas_west[j]; }
						default: { SetFailState("Unknown dir: %d", dir); }
					}
					if (connection == 0) {
						SetFailState("Conn == 0");
					}
					if (connection == start_area[0].id) {
						found_solution = true;
					}
				}
			}
			
			//PrintToServer("Was prior to reassign: %d", iter_id);
			iter_id = iter_area[0].NearestConnection(solution_path_ids, num_solution_ids);
			
			// Something went wrong with solving, just return what we've got thus far
			if (iter_id == 0) {
				return num_solution_ids;
			}
			
			//PrintToServer("Iter id is prepared as: %d", iter_id);
			for (int i = 0; i < num_solution_ids; ++i) {
				if (iter_id == solution_path_ids[i]) {
					SetFailState("iter_id (%d) already in solution_path_ids", iter_id);
				}
				//PrintToServer("solution_path_ids %d: %d", i, solution_path_ids[i]);
			}
			solution_path_ids[num_solution_ids++] = iter_id;
		}
	}
	
	if (found_solution) {
		--num_solution_ids;
		if (num_solution_ids <= 0) {
			SetFailState("found_solution == true, but num_solution_ids == 0");
		}
		
		//PrintToServer("Found solution!");
		return num_solution_ids;
	}
	
	return 0;
}

stock bool AreNeighbours(const int self_id, const int other_id)
{
	NavArea self_area[1];
	GetAreaById(self_id, self_area);
	
	for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int i = 0; i < self_area[0].num_connected_areas[dir]; ++i) {
			int connection = 0;
			switch (dir) {
				case NORTH: { connection = self_area[0].connected_areas_north[i]; }
				case EAST: { connection = self_area[0].connected_areas_east[i]; }
				case SOUTH: { connection = self_area[0].connected_areas_south[i]; }
				case WEST: { connection = self_area[0].connected_areas_west[i]; }
				default: { SetFailState("Unknown direction %d", dir); }
			}
			
			if (connection == 0) {
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				SetFailState("Failed to get %sern connection for id %d", dirnames[dir], self_area[0].id);
			}
			
			if (connection == other_id) {
				return true;
			}
		}
	}
	return false;
}

stock int NearestConnection(const int self_id, const int[] excludes, const int num_excludes)
{
	int nearest_connection = 0;
	float nearest_distance = 0.0;
	
	NavArea area[1];
	GetAreaById(self_id, area);
		
	for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int i = 0; i < area[0].num_connected_areas[dir]; ++i) {
			int connection = 0;
			switch (dir) {
				case NORTH: { connection = area[0].connected_areas_north[i]; }
				case EAST: { connection = area[0].connected_areas_east[i]; }
				case SOUTH: { connection = area[0].connected_areas_south[i]; }
				case WEST: { connection = area[0].connected_areas_west[i]; }
				default: { SetFailState("Unknown direction %d", dir); }
			}
			
			if (connection == 0) {
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				SetFailState("Failed to get %sern connection for id %d", dirnames[dir], area[0].id);
			}
			
			//PrintToServer("Found connection: %d");
			
			bool skip_this = false;
			for (int j = 0; j < num_excludes; ++j) {
				if (connection == excludes[j]) {
					skip_this = true;
					break;
				}
			}
			
			if (!skip_this) {
				float distance = area[0].DistanceTo(connection);
				
				if (nearest_connection == 0 || distance < nearest_distance) {
					nearest_connection = connection;
					nearest_distance = distance;
				}
			}
		}
	}
	
	return nearest_connection;
}

stock int NearestArea(const int self_id, const int[] excludes, const int num_excludes)
{
	int nearest_area = 0;
	float nearest_distance = 0.0;
	
	NavArea area;
	int num_iterated = 0;
	for (int i = _areas.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		_areas.GetArray(i, area);
		for (int j = 0; j < num_excludes; ++j) {
			if (area.id == excludes[i]) {
				continue;
			}
		}
		float distance = area.DistanceTo(self_id);
		if (nearest_area == 0 || distance < nearest_distance) {
			nearest_area = area.id;
			nearest_distance = distance;
		}
	}
	
	return nearest_area;
}

stock int GetConnections(const NavArea area, int[] connections, const int max_connections)
{
	int num_connection = 0;
	for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int j = 0; j < area.num_connected_areas[dir]; ++j) {
			int connection = -1;
			switch (dir)
			{
				case DIR_NORTH: { connection = area.connected_areas_north[j]; }
				case DIR_EAST: { connection = area.connected_areas_east[j]; }
				case DIR_SOUTH: { connection = area.connected_areas_south[j]; }
				case DIR_WEST: { connection = area.connected_areas_west[j]; }
			}
			connections[num_connection++] = connection;
		}
	}
	return num_connections;
}

stock void AddArea(const NavArea area)
{
	TheNavAreaList.PushArray(area);
	++num_areas_in_the_nav_list;
}

stock int GetNumAreas()
{
	return num_areas_in_the_nav_list;
}

stock bool GetAreaById(const int id, NavArea[1] out)
{
	NavArea area;
	int num_iterated = 0;
	for (int i = TheNavAreaList.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		TheNavAreaList.GetArray(i, area);
		if (area.id == id) {
			out[0] = area;
			return true;
		}
	}
	return false;
}

stock bool GetNearestArea(const float pos[3], NavArea[1] out_area)
{
	float nearest_distance = -1.0;
	
	int num_iterated = 0;
	for (int i = TheNavAreaList.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		NavArea this_area;
		TheNavAreaList.GetArray(i, this_area);
		
		float this_distance = GetVectorDistance(this_area.centroid, pos);
		
		if (nearest_distance == -1 || this_distance < nearest_distance) {
			out_area[0] = this_area;
			nearest_distance = this_distance;
		}
	}
	return nearest_distance != -1;
}

stock int GetNearestOfAreaList(const int this_area_id, const int[] area_list, const int num_areas,
	int& nearest_array_index)
{
	NavArea area[1];
	if (!GetAreaById(this_area_id, area)) {
		SetFailState("!GetAreaById");
	}
	
	int nearest = 0;
	float nearest_distance;
	
	for (int i = 0; i < num_areas; ++i) {
		float distance = area[0].DistanceTo(area_list[i]);
		if (nearest == 0 || distance < nearest_distance) {
			//PrintToServer("%f < %f", distance, nearest_distance);
			nearest = area_list[i];
			nearest_distance = distance;
			nearest_array_index = i;
		} else if (distance >= nearest_distance) {
			//PrintToServer("%f >= %f", distance, nearest_distance);
		}
	}
	
	return nearest;
}

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
stock bool IsBotClient(int client)
{
    return client != 0 && IsFakeClient(client) && !IsClientSourceTV(client);
}

// Predicate for client position initiated ray trace.
stock bool DidNotHitSelf(int self, int contentsMask, int target)
{
    return self != target;
}

stock void MultiplyVectors(const float v1[3], const float v2[3], float res[3])
{
    res[0] = v1[0] * v2[0];
    res[1] = v1[1] * v2[1];
    res[2] = v1[2] * v2[2];
}

stock bool VectorsEqual(const float[3] v1, const float[3] v2, const float max_ulps = 0.0)
{
	// Needs to exactly equal.
	if (max_ulps == 0) {
		return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
	}
	// Allow an inaccuracy of size max_ulps.
	else {
		if (FloatAbs(v1[0] - v2[0]) > max_ulps) { return false; }
		if (FloatAbs(v1[1] - v2[1]) > max_ulps) { return false; }
		if (FloatAbs(v1[2] - v2[2]) > max_ulps) { return false; }
		return true;
	}
}

stock float Lerp(float a, float b, float scale = 0.0)
{
    if (scale == 0) {
        scale = GetGameFrameTime();
    }
    return a + (b - a) * scale;
}

stock bool GetPlannedPosition(float[3] pos)
{
	if (s_movementPlanStack == -1) {
		return false;
	}
	
	if (s_movementPlan[s_movementPlanStack] == 0) {
		SetFailState("Invalid movement plan id in stack pos %d", s_movementPlanStack);
	}
	
	NavArea area[1];
	if (!GetAreaById(s_movementPlan[s_movementPlanStack], area)) {
		return false;
	}
	
	pos = area[0].centroid;
	return true;
}

stock int teamidx_to_navteamidx(const int team_index)
{
	return team_index == TEAM_NSF ? NAV_TEAM_IDX_NSF : NAV_TEAM_IDX_JINRAI;
}

stock int navteamidx_to_teamidx(const int bot_mesh_team_index)
{
	return bot_mesh_team_index == NAV_TEAM_IDX_NSF ? TEAM_NSF : TEAM_JINRAI;
}

#endif // _NT_BOTS_BASE_INC_