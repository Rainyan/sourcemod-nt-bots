#if !defined(_NT_PATHFIND_INC_)
#define _NT_PATHFIND_INC_

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
stock bool NavAreaBuildPath(const NavArea start_area, const NavArea goal_area, float[3] goal_pos,
	Function cost_function, NavArea closest_area)
{
	// if we are already in the goal area, build trivial path
	if (start_area.id == goal_area.id)
	{
		goal_area.SetParent(0);
		return true;
	}
	
	float actual_goal_pos[3];
	if (!VectorsEqual(goal_pos, vec3_origin)) {
		actual_goal_pos = goal_pos;
	} else {
		goal_area.GetOrigin(actual_goal_pos);
	}
	
	
	
	return false;
}

#endif // _NT_PATHFIND_INC_