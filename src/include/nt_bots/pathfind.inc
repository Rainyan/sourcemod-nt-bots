#if !defined(_NT_PATHFIND_INC_)
#define _NT_PATHFIND_INC_

// Cost functor used with NavAreaBuildPath
float ShortestPathCost(const int area_id, const int from_area_id /*, const NavLadder ladder*/)
{
	if (from_area_id == 0) {
		// first area in path, no cost
		return 0.0;
	}
	
	NavArea area[1];
	if (!GetAreaById(area_id, area)) {
		SetFailState("Failed to get NavArea");
	}
	
	NavArea from_area[1];
	GetAreaById(from_area_id, from_area);
	
	// compute distance travelled along path so far
	float distance = area[0].DistanceTo(from_area[0].id);
	
	float cost = distance + from_area[0].cost_so_far;
	
	if (area[0].ShouldCrouchHere()) {
#define CROUCH_PENALTY 20.0
		cost += CROUCH_PENALTY * distance;
	}
	
	if (area[0].ShouldJumpHere()) {
#define JUMP_PENALTY 5.0
		cost += JUMP_PENALTY * distance;
	}
	
	return cost;
}

/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * Returns true if a path exists.
 */
bool NavAreaBuildPath(const int start_area_id, const int goal_area_id = 0, int& closest_area_id,
	const float goal_pos_x = 0.0, const float goal_pos_y = 0.0, const float goal_pos_z = 0.0)
{
	if (HasActivePlan()) {
		PrintToServer("Has active plan; skipping pathfind"); // FIXME: shouldnt call unnecessarily
		return false;
	}
	
	if (start_area_id == 0) {
		SetFailState("Invalid start_area_id");
	}
	NavArea start_area[1];
	if (!GetAreaById(start_area_id, start_area)) {
		SetFailState("Failed to get NavArea start_area");
	} else if (start_area[0].id == 0) {
		SetFailState("Invalid start_area.id");
	}
	
	NavArea goal_area[1];
	if (goal_area_id != 0) {
		GetAreaById(goal_area_id, goal_area);
	}
	
	// if we are already in the goal area, build trivial path
	if (start_area[0].id == goal_area_id)
	{
		closest_area_id = goal_area_id;
		goal_area[0].SetParent(0);
		return true;
	}
	
	float goal_pos[3];
	goal_pos[0] = goal_pos_x;
	goal_pos[1] = goal_pos_y;
	goal_pos[2] = goal_pos_z;
	
	// determine actual goal position
	float actual_goal_pos[3];
	if (!VectorsEqual(goal_pos, vec3_origin)) {
		actual_goal_pos = goal_pos;
	} else {
		actual_goal_pos = goal_area[0].centroid;
	}
	
	// start search
	ClearSearchLists();
	
	// compute estimate of path length
	start_area[0].total_cost = GetVectorDistance(start_area[0].centroid, actual_goal_pos);
	
	float initial_cost = ShortestPathCost(start_area[0].id, 0);
	// Should never happen with the default functor, but keeping for extendibility
	if (initial_cost < 0) {
		PrintToServer("#1 FUNCTOR FAILED!");
		return false;
	}
	start_area[0].cost_so_far = initial_cost;
	
	PrintToServer("Pre-entry: %d", s_open_list_id);
	start_area[0].AddToOpenList();
	PrintToServer("Post-entry: %d", s_open_list_id);
	
	// keep track of the area we visit that is closest to the goal
	closest_area_id = start_area[0].id;
	float closest_area_distance = start_area[0].total_cost;
	
	bool had_goal_pos = (!(VectorsEqual(goal_pos, vec3_origin)));
	
	int safety_bailout = 0;
	int safety_bailout_limit = 1000;
	
	PrintToServer("Pre-pathfind: %d", s_open_list_id);
	
	// pathfinding
	while (!IsOpenListEmpty()) {
		if (safety_bailout++ >= safety_bailout_limit) {
			SetFailState("BAILING OUT");
		}
		
		PrintToServer("s_open_list_id: %d", s_open_list_id);
		
		// get next area to check
		NavArea iter_area[1];
		if (!PopOpenList(iter_area)) {
			PrintToServer("!PopOpenList");
			return false;
		} else if (iter_area[0].id == 0) {
			SetFailState("Popped invalid NavArea");
		}
		
		// don't consider blocked areas
		if (iter_area[0].is_blocked) {
			continue;
		}
		
		// check if we have found the goal area or position
		if ((iter_area[0].id == goal_area_id) ||
			(goal_area_id == 0 && had_goal_pos && iter_area[0].Contains(goal_pos)))
		{
			PrintToServer("Iter area before return: %d", iter_area[0].id);
			
			closest_area_id = iter_area[0].id;
			return true;
		}
		
		// search adjacent areas
		for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
			for (int i = 0; i < iter_area[0].num_connected_areas[dir]; ++i) {
				int new_area_id = 0;
				switch (dir) {
					case NORTH: { new_area_id = iter_area[0].connected_areas_north[i]; }
					case EAST: { new_area_id = iter_area[0].connected_areas_east[i]; }
					case SOUTH: { new_area_id = iter_area[0].connected_areas_south[i]; }
					case WEST: { new_area_id = iter_area[0].connected_areas_west[i]; }
					default: { SetFailState("Unknown direction %d", dir); }
				}
				if (new_area_id == 0) {
					SetFailState("Area id was invalid");
				}
				
				// don't backtrack
				if (new_area_id == iter_area[0].id) {
					continue;
				}
				
				NavArea new_area[1];
				if (!GetAreaById(new_area_id, new_area)) {
					SetFailState("Failed to get area");
				}
				
				// don't consider blocked areas
				if (new_area[0].is_blocked) {
					continue;
				}
				
				float new_cost_so_far = ShortestPathCost(new_area[0].id, iter_area[0].id);
				
				// check if cost functor says this area is a dead-end
				if (new_cost_so_far < 0.0) {
					continue;
				}
				
				if (/*(new_area[0].IsOpen() || new_area[0].IsClosed()) &&*/
					new_area[0].cost_so_far <= new_cost_so_far)
				{
					// this is a worse path - skip it
					continue;
				}
				
				// compute estimation of remaining distance
				float new_cost_remaining = GetVectorDistance(new_area[0].centroid, actual_goal_pos);
				
				// track closest area to goal in case path fails
				if (new_cost_remaining < closest_area_distance) {
					closest_area_id = new_area_id;
					closest_area_distance = new_cost_remaining;
				}
				
				new_area[0].SetParent(iter_area[0].id, view_as<NavTraverseType>(dir));
				new_area[0].cost_so_far = new_cost_so_far;
				new_area[0].total_cost = new_cost_so_far + new_cost_remaining;
				
				if (new_area[0].IsOpen()) {
					PrintToServer("%d-->%d IS OPEN", iter_area[0].id, new_area[0].id);
					// area already on open list, update the list order to keep costs sorted
					new_area[0].UpdateOnOpenList();
				} else {
					PrintToServer("%d-->%d NOT IS OPEN", iter_area[0].id, new_area[0].id);
					new_area[0].AddToOpenList();
				}
			}
		}
		iter_area[0].AddToClosedList();
	}
	
	PrintToServer("PATHFIND FELL THROUGH WHILE (%d iterations)", safety_bailout);
	return false;
}

#endif // _NT_PATHFIND_INC_