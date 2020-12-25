#pragma semicolon 1

#include <sourcemod>

#define PLUGIN_VERSION "0.1"

public Plugin myinfo = {
	name = "NT Dynamic Bot Spawner",
	description = "Dynamically adjust server bot amount, depending on number of players connected.",
	author = "Rain",
	version = PLUGIN_VERSION,
	url = "https://github.com/Rainyan/sourcemod-nt-bots"
};

static int _num_humans;
static int _num_bots;

ConVar g_cEnabled = null;
ConVar g_cPlayerMinLimit = null;

public void OnPluginStart()
{
	CreateConVar("sm_nt_dynamic_bot_spawner_version", PLUGIN_VERSION, "NT Bots Dynamic Spawner version.", FCVAR_DONTRECORD);
	
	g_cEnabled = CreateConVar("sm_nt_dynamic_bot_spawner_enabled", "1", "Whether to automatically adjust amount of bots.", _, true, 0.0, true, 1.0);
	g_cPlayerMinLimit = CreateConVar("sm_nt_dynamic_bot_spawner_player_min_limit", "6", "Try to keep this many players (bots or humans) on the server, at minimum.", _, true, 0.0, true, 32.0);
}

public void OnMapStart()
{
	CreateTimer(1.0, Timer_CheckBotAmount, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

public void OnConfigsExecuted()
{
	static bool first_launch = true;
	if (first_launch) {
		for (int client = 1; client <= MaxClients; ++client) {
			if (!IsClientInGame(client)) {
				continue;
			}
			else if (!IsFakeClient(client)) {
				++_num_humans;
			}
			else if (IsBotClient(client)) {
				++_num_bots;
			}
		}
		CheckBotAmount();
		
		first_launch = false;
	}
}

public void OnClientPutInServer(int client)
{
	if (!IsFakeClient(client)) {
		++_num_humans;
	}
	else if (IsBotClient(client)) {
		++_num_bots;
	}
}

public void OnClientDisconnect(int client)
{
	if (!IsFakeClient(client)) {
		--_num_humans;
	}
	else if (IsBotClient(client)) {
		--_num_bots;
	}
}

public Action Timer_CheckBotAmount(Handle timer)
{
	CheckBotAmount();
	return Plugin_Continue;
}

void CheckBotAmount()
{
	if (!g_cEnabled.BoolValue) {
		return;
	}
	
	int players_delta = (_num_bots + _num_humans) - g_cPlayerMinLimit.IntValue;
	
	// Too few players?
	if (players_delta < 0) {
		SpawnNewBot_1();
	}
	// Too many bots?
	else if (players_delta > 0 && _num_bots > 0) {
		RemoveBot();
	}
}

void SpawnNewBot_1()
{
	// Need to delay re-adding cheats flag on the bot_add command because
	// these can't be chained into a guaranteed exec order.
	// It's a bit unelegant, but the worst case here is someone spawning
	// an extra bot that then gets kicked by this plugin shortly after,
	// so it should be acceptable.
	SetCommandFlags("bot_add", GetCommandFlags("bot_add") & ~FCVAR_CHEAT);
	CreateTimer(0.0, Timer_SpawnNewBot2, _, TIMER_FLAG_NO_MAPCHANGE); // guaranteed accurate only up to 100 ms, so we do timers of 0 and 300 ms
	CreateTimer(0.3, Timer_SpawnNewBot3);
}

public Action Timer_SpawnNewBot2(Handle timer)
{
	ServerCommand("bot_add");
	return Plugin_Stop;
}

public Action Timer_SpawnNewBot3(Handle timer)
{
	SetCommandFlags("bot_add", GetCommandFlags("bot_add") | FCVAR_CHEAT);
	return Plugin_Stop;
}

void RemoveBot()
{
	for (int client = 1; client <= MaxClients; ++client) {
		if (IsClientInGame(client) && IsBotClient(client)) {
			KickClient(client);
			break;
		}
	}
}

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
stock bool IsBotClient(int client)
{
    return client != 0 && IsFakeClient(client) && !IsClientSourceTV(client);
}
