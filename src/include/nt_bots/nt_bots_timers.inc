#if defined(_NT_BOTS_TIMERS_INC_)
#endinput
#endif
#define _NT_BOTS_TIMERS_INC_

// Generic timer for repeat updates at slower frequency
public Action Timer_Generic(Handle timer, any data)
{
	for (int client = 1; client <= MaxClients; ++client) {
		if (!IsClientInGame(client) || !IsBotClient(client) || !IsPlayerAlive(client)) {
			continue;
		}
		
		TimedThink(client);
	}
	
	return Plugin_Continue;
}

static void TimedThink(int client)
{
	if (AreOnlyBotsAlive()) {
		MakePlan(client, true);
	}
	else if (IsGameModeKillConfirmed()) {
		if (GetGameTime() - _enemy_last_spotted_time_single[client] > g_cReactionTime.FloatValue) {
			for (int other_client = 1; other_client <= MaxClients; ++other_client) {
				_enemy_last_spotted_time[client][other_client] = 0.0;
			}
			MakePlan(client, true);
		}
	}
	else {
		if (EnemyHasGhost(client) || (!HasActivePlan(client) && !IsNearPlannedPosition(client))) {
			MakePlan(client, true);
		}
	}
	
	bot_last_took_dmg_from[client] = 0;
}
