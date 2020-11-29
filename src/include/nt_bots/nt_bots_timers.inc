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
	if (HasActivePlan()) {
		return;
	}
	
	float client_pos[3];
	GetClientAbsOrigin(client, client_pos);
	
	if (VectorsEqual(s_targetPos, vec3_origin)) {
		s_targetPos = client_pos;
		return;
	}
	
	NavArea this_area[1];
	if (!GetNearestArea(client_pos, this_area)) {
		return;
	} else if (this_area[0].id == 0) {
		SetFailState("This area id == 0");
	}
	
	NavArea target_area[1];
	if (!GetNearestArea(s_targetPos, target_area)) {
		return;
	}
	
	int closest_area_id = 0;
	if (NavAreaBuildPath(this_area[0].id, target_area[0].id, closest_area_id)) {
		PrintToServer("%f Solved target area as: %d", GetGameTime(), closest_area_id);
		
		// This should never happen
		if (closest_area_id == 0) {
			SetFailState("NavPath succeeded but passed id 0");
		}
		
		if (closest_area_id == target_area[0].id) {
			s_targetPos = target_area[0].centroid;
		} else {
			NavArea closest_area[1];
			if (!GetAreaById(closest_area_id, closest_area)) {
				SetFailState("Failed to get closest NavArea: %d", closest_area_id);
			}
			
			ClearPlan();
			PlanMove(closest_area[0].id); // TODO: plan multiple moves for the entire route
			
			// TODO: debug check, remove this later
			if (!HasActivePlan()) {
				SetFailState("Failed to set plan");
			}
		}
	} else {
		PrintToServer("%f No path found from %d -> %d", GetGameTime(), this_area[0].id, target_area[0].id);
	}
}

#endif // _NT_BOTS_TIMERS_INC_