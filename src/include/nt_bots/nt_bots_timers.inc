#if !defined(_NT_BOTS_TIMERS_INC_)
#define _NT_BOTS_TIMERS_INC_

// Generic timer placeholder for repeat updates
// TODO: Is this needed? Relevant hook replacements?
public Action Timer_Generic(Handle timer, any data)
{
	for (int client = 1; client <= MaxClients; ++client) {
		if (!IsClientConnected(client) || !IsBotClient(client)) {
			continue;
		}
		TimedThink(client);
	}
	return Plugin_Continue;
}

void TimedThink(int client)
{
	float client_pos[3];
	GetClientAbsOrigin(client, client_pos);
	
	if (VectorsEqual(s_targetPos, vec3_origin)) {
		s_targetPos = client_pos;
		return;
	}
	
	Area this_area[1];
	if (!GetNearestArea(client_pos, this_area)) {
		return;
	} else if (this_area[0].id == 0) {
		SetFailState("This area id == 0");
	}
	
	Area target_area[1];
	if (!GetNearestArea(s_targetPos, target_area)) {
		return;
	}
	
	if (s_movementPlanStack >= 0) {
		if (!this_area[0].IsNeighbourOf(s_movementPlan[s_movementPlanStack])) {
			int nearest_index = -1;
			int nearest = GetNearestOfAreaList(this_area[0].id, s_movementPlan,
				s_movementPlanStack + 1, nearest_index);
			if (nearest == s_movementPlan[s_movementPlanStack]) {
#if(0)
				PrintToServer("%d is not a neighbour of next (and nearest) movement plan node %d; \
resetting the plan stack.", this_area[0].id, s_movementPlan[s_movementPlanStack]);
#endif
				s_movementPlanStack = -1;
				return;
			} else {
				if (nearest_index == -1) {
					SetFailState("Nearest index was -1");
				}
				int swap = s_movementPlan[s_movementPlanStack];
				s_movementPlan[s_movementPlanStack] = nearest;
				s_movementPlan[nearest_index] = swap;
				//PrintToServer("Swapping indices %d <-> %d", s_movementPlanStack, nearest_index);
			}
		}
		
		float waypoint_pos[3];
		if (!GetPlannedPosition(waypoint_pos)) {
			return;
		}
		
		//PrintToServer("Moving %d -> (%d) -> %d (stack: %d)",
		//	this_area[0].id,
		//	s_movementPlan[s_movementPlanStack],
		//	target_area[0].id,
		//	s_movementPlanStack);
		
		return;
	}
	
	int path_solution[MAX_AREA_MOVES_PLANNED_FOR_FUTURE];
	int num_nodes = this_area[0].SolvePathFor(target_area[0].id,
		path_solution, MAX_AREA_MOVES_PLANNED_FOR_FUTURE);
	
	if (num_nodes == 0) {
		s_movementPlanStack = -1;
		return;
	}
	
	// This should never happen
	if (num_nodes > MAX_AREA_MOVES_PLANNED_FOR_FUTURE) {
		SetFailState("Solved path in %d moves, when asked for %d max.",
			num_nodes, MAX_AREA_MOVES_PLANNED_FOR_FUTURE);
	}
	
	//PrintToServer("Solved path %d -> %d in %d nodes (%d max).",
	//	this_area[0].id, target_area[0].id,
	//	num_nodes, MAX_AREA_MOVES_PLANNED_FOR_FUTURE);
	
	for (s_movementPlanStack = 0; s_movementPlanStack < num_nodes; ++s_movementPlanStack) {
		//PrintToServer("Movement stack assign: %d", s_movementPlanStack);
		s_movementPlan[s_movementPlanStack] = path_solution[s_movementPlanStack];
	}
	s_movementPlanStack = num_nodes - 1;
	//PrintToServer("Movement stack finalized pos is: %d", s_movementPlanStack);
}

#endif // _NT_BOTS_TIMERS_INC_