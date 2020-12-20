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
	if (!HasActivePlan(client) && !IsNearPlannedPosition(client)) {
		MakePlan(client, true);
	}
	
	bot_last_took_dmg_from[client] = 0;
}
