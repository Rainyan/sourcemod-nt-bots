#if !defined(_NT_BOTS_NATIVES_INC_)
#define _NT_BOTS_NATIVES_INC_

public void OnPluginStart()
{
	CreateConVar("sm_nt_bots_version", PLUGIN_VERSION,
		"NT Bots plugin version.", FCVAR_DONTRECORD);
	
	RegConsoleCmd("sm_nt_bots_order_pos", Cmd_OrderPos);
	RegConsoleCmd("sm_nt_bots_place", Cmd_SetPos);
	
	g_cAngleOffset = CreateConVar("sm_nt_bots_ang_offset", "0");
	g_cBitsOverride = CreateConVar("sm_nt_bots_bits", "0");
	
	CreateTimer(BOT_TIMED_THINK_INTERVAL, Timer_Generic, _, TIMER_REPEAT);
	
	TheNavMesh.Initialize();
}

public void OnMapStart()
{
	rayIndex = PrecacheModel("sprites/purplelaser1.spr", true);
	if (rayIndex == 0) {
		SetFailState("Failed to precache model");
	}
	
	if (!TheNavMesh.LoadCurrentMap()) {
		SetFailState("Failed to load nav mesh for current map; shutting down"); // TODO: shut down grafecully
	}
}

public void OnClientPutInServer(int client)
{
	if (IsBotClient(client)) {
		if (!SDKHookEx(client, thinkHookType, ThinkHook)) {
			SetFailState("SDKHook unsuccessful");
		}
	}
}

public void OnClientDisconnect(int client)
{
	if (IsBotClient(client)) {
		SDKUnhook(client, thinkHookType, ThinkHook);
	}
}

public Action OnPlayerRunCmd(int client, int& buttons, int& impulse,
	float vel[3], float angles[3], int& weapon, int& subtype,
	int& cmdnum, int& tickcount, int& seed, int mouse[2])
{
#define TEAM_NONE		0
#define TEAM_SPECTATOR	1
#define TEAM_JINRAI 	2
#define TEAM_NSF		3	
	if ((!IsBotClient(client)) ||
		// Only override on bots that have joined a team
		(GetClientTeam(client) <= TEAM_SPECTATOR) ||
		(!IsPlayerAlive(client)))
	{
		return Plugin_Continue;
	}
	
	impulse = 0;
	buttons = g_cBitsOverride.IntValue;

	buttons |= DecideVelocity(client, vel);
	angles = s_ang;
	
	return Plugin_Continue;
}

#endif // _NT_BOTS_NATIVES_INC_