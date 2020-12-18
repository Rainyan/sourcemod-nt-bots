#if defined(_NT_BOTS_TIMERS_INC_)
#endinput
#endif
#define _NT_BOTS_TIMERS_INC_

// Generic timer for repeat updates at slower frequency
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

static void TimedThink(int client)
{
	if (!HasActivePlan(client)) {
		MakePlan(client);
		
		float client_pos[3];
		GetClientAbsOrigin(client, client_pos);
		
		float next_pos[3];
		if (GetPlannedPosition(client, next_pos)) {
			RequestPath(client, client_pos, next_pos);
		}
	}
}
