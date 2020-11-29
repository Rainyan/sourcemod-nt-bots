#if !defined(_NT_PATHFIND_INC_)
#define _NT_PATHFIND_INC_

// Cost functor used with NavAreaBuildPath
float ShortestPathCost(const NavArea area, const int from_area_id /*, const NavLadder ladder*/)
{
	if (from_area_id == 0) {
		// first area in path, no cost
		return 0.0;
	}
	
	Area from_area[1];
	GetAreaById(from_area_id, from_area);
	
	// compute distance travelled along path so far
	float distance = area.DistanceTo(from_area);
	
	float cost = distance + from_area.cost_so_far;
	
	if (area.ShouldCrouchHere()) {
#define CROUCH_PENALTY 20.0
		cost += CROUCH_PENALTY * dist;
	}
	
	if (area.ShouldJumpHere()) {
#define JUMP_PENALTY 5.0
		cost += JUMP_PENALTY * dist;
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
bool NavAreaBuildPath(const NavArea start_area, const int goal_area_id = 0, int& closest_area_id = 0,
	const float goal_pos_x = 0.0, const float goal_pos_y = 0.0, const float goal_pos_z = 0.0)
{
	// if we are already in the goal area, build trivial path
	if (start_area.id == goal_area.id)
	{
		goal_area.SetParent(0);
		return true;
	}
	
	// determine actual goal position
	float actual_goal_pos[3];
	if (!VectorsEqual(goal_pos, vec3_origin)) {
		actual_goal_pos = goal_pos;
	} else {
		actual_goal_pos = goal_area.centroid;
	}
	
	// start search
	ClearSearchLists();
	
	// compute estimate of path length
	start_area.total_cost = GetVectorDistance(start_area.centroid, actual_goal_pos);
	
	float initial_cost = ShortestPathCost(start_area, 0);
	// Should never happen with the default functor, but keeping for extendibility
	if (initial_cost < 0) {
		return false;
	}
	start_area.cost_so_far = initial_cost;
	
	start_area.AddToOpenList();
	
	// keep track of the area we visit that is closest to the goal
	if (closest_area_id != 0) {
		closest_area_id = start_area.id;
	}
	float closest_area_distance = start_area.total_cost;
	
	float goal_pos[3] = { goal_pos_x, goal_pos_y, goal_pos_z };
	bool had_goal_pos = (!(VectorsEqual(goal_pos, vec3_origin)));
	
	// pathfinding
	while (s_open_list_id != 0) {
		// get next area to check
		NavArea iter_area[1];
		if (!PopOpenList(iter_area)) {
			return false;
		}
		
		// don't consider blocked areas
		if (iter_area[0].is_blocked) {
			continue;
		}
		
		// check if we have found the goal area or position
		if ((iter_area[0].id == goal_area_id) ||
			(goal_area_id == 0 && had_goal_pos && iter_area[0].Contains(goal_pos)))
		{
			closest_area_id = iter_area[0].id;
			return true;
		}
		
		// TODO
	}
	
	return false;
}

#endif // _NT_PATHFIND_INC_