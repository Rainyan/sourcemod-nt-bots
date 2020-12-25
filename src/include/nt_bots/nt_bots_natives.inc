#if defined(_NT_BOTS_NATIVES_INC_)
#endinput
#endif
#define _NT_BOTS_NATIVES_INC_

public void OnPluginStart()
{
	server_tick_rate = RoundToNearest(1 / GetTickInterval());
	
	if (!HookEventEx("player_spawn", OnPlayerSpawn, EventHookMode_Post)) {
		SetFailState("Failed to hook event player_spawn");
	}
	else if (!HookEventEx("player_hurt", OnPlayerHurt, EventHookMode_Pre)) {
		SetFailState("Failed to hook event player_hurt");
	}
	else if (!HookEventEx("game_round_start", OnRoundStart, EventHookMode_Post)) {
		SetFailState("Failed to hook event game_round_start");
	}
	
	CreateConVar("sm_nt_bots_version", PLUGIN_VERSION,
		"NT Bots plugin version.", FCVAR_DONTRECORD);
	
	RegConsoleCmd("sm_nt_bots_order_pos", Cmd_OrderPos);
	RegConsoleCmd("sm_nt_bots_place", Cmd_SetPos);
	
	RegConsoleCmd("sm_tie", Cmd_BotsTie);
	
	g_cBitsOverride = CreateConVar("sm_nt_bots_bits", "0");
	
	g_cAimInaccuracy = CreateConVar("sm_nt_bots_aim_inaccuracy", "25.0", "Scale of bot aim inaccuracy.",
		_, true, 0.0, true, 1000.0);
	
	g_cReactionTime = CreateConVar("sm_nt_bots_reaction_time", "0.16", "Visual reaction time of the bots, in seconds.",
		_, true, 0.0, true, 1.0);
	
	g_cSuppressFireTime = CreateConVar("sm_nt_bots_suppress_fire_duration", "0.33", "How long to keep firing after losing sight of enemy, in seconds.",
		_, true, 0.0, true, 10.0);
	
	g_cForgetEnemyTime = CreateConVar("sm_nt_bots_forget_enemy_time", "5.0", "\"Forget\" about a spotted enemy after this many seconds.",
		_, true, 0.0, true, 180.0);
	
	g_cBotAimSpeed = CreateConVar("sm_nt_bots_aim_speed", "5.0", "How fast should the bot acquire target when initially aiming elsewhere.",
		_, true, 0.1, true, 100.0);
	
	g_cNoTarget = CreateConVar("sm_nt_bots_notarget", "0.0", "Debug command. Make the bots not react to enemies.",
		_, true, 0.0, true, 1.0);
	
	g_cMaxSpinSpeed = CreateConVar("sm_nt_bots_max_spin_speed", "1.0", "Absolute maximum speed scaler for how fast the bot can spin around. Set to 0 to disable limiting.",
		_, true, 0.0, true, 1.0);
	
	g_cDrawDebugBeams = CreateConVar("sm_nt_bots_debug_draw_beams", "0.0", "Whether to draw pathing routes etc. using laser beam effects. Debug cvar.",
		_, true, 0.0, true, 1.0);
	
	g_cAlwaysKillConfirmedMode = CreateConVar("sm_nt_bots_debug_always_killconfirmed_mode", "0.0", "Whether to always use the TDM/\"Kill Confirmed\" behaviour rules. Debug cvar.",
		_, true, 0.0, true, 1.0);
	
	g_cBotClass = FindConVar("bot_class");
	if (g_cBotClass == null) {
		SetFailState("Failed to find native cvar bot_class");
	}
	
	g_cIsKillConfirmedMode = FindConVar("nt_tdm_enabled");
}

public void OnConfigsExecuted()
{
	ConnectToDatabase();
}

public Action Cmd_BotsTie(int client, int argc)
{
	// 15 seconds of freeze time late spawning for humans,
	// and idle players might take another 15 seconds before auto-spawning.
	// So wait for them while they appear "dead".
	int wait_at_least_secs = 30;
	if (round_start_time == 0 || GetGameTime() - round_start_time < wait_at_least_secs) {
		ReplyToCommand(client, "%s Please wait at least %d seconds from the beginning of the round.", BOTS_MSG_PREFIX, wait_at_least_secs);
		return Plugin_Handled;
	}
	
	int num_dead_playing_humans, num_alive_playing_bots;
	
	for (int iter_client = 1; iter_client <= MaxClients; ++iter_client) {
		if (!IsClientInGame(iter_client)) {
			continue;
		}
		if (!IsBotClient(iter_client)) {
			if (GetClientTeam(iter_client) > TEAM_SPECTATOR && !IsPlayerAlive(iter_client)) {
				++num_dead_playing_humans;
			}
		}
		else if (IsPlayerAlive(iter_client)) {
			++num_alive_playing_bots;
		}
	}
	
	if (num_alive_playing_bots == 0) {
		ReplyToCommand(client, "%s No living bots found to use this command on.", BOTS_MSG_PREFIX);
	}
	else if (num_dead_playing_humans > 0 && num_alive_playing_bots > 0) {
		SoftSlayAllBots();
		PrintToChatAll("%s Only bots remaining. Ending the round in a tie.", BOTS_MSG_PREFIX);
	}
	else {
		ReplyToCommand(client, "%s Cannot use this command right now. Need to have only bot players alive, and human players in at least one team.", BOTS_MSG_PREFIX);
	}
	
	return Plugin_Handled;
}

public void OnMapStart()
{
	round_start_time = 0.0;
	
	UpdateDbTableNamesForThisMap();
	
	rayIndex = PrecacheModel("sprites/purplelaser1.spr", true);
	if (rayIndex == 0) {
		SetFailState("Failed to precache model");
	}
	
	for (int client = 1; client <= MaxClients; ++client) {
		if (IsClientInGame(client) && IsBotClient(client)) {
			BotSetup(client);
		}
	}
	
	RoundStartActions();
}

public void OnMapEnd()
{
	if (g_hGenericTimer != null) {
		delete g_hGenericTimer;
		g_hGenericTimer = null;
	}
}

void BotSetup(int client)
{
	ClearPlan(client);
	
	if (!SDKHookEx(client, thinkHookType, ThinkHook)) {
		SetFailState("ThinkHook hook unsuccessful");
	}
	
	AddToArr(bot_clients, num_bot_clients, client);
}

public void OnClientPutInServer(int client)
{
	if (IsBotClient(client)) {
		BotSetup(client);
	}
}

public void OnClientDisconnect(int client)
{
	if (IsBotClient(client)) {
		SDKUnhook(client, thinkHookType, ThinkHook);
		RemoveFromArr(bot_clients, num_bot_clients, client);
	}
}

public void OnGameFrame()
{
	SetNextBotThinkerHead();
}

static float prev_vel[NEO_MAX_PLAYERS + 1][3];
static float prev_angles[NEO_MAX_PLAYERS + 1][3];

public Action OnPlayerRunCmd(int client, int& buttons, int& impulse,
	float vel[3], float angles[3], int& weapon, int& subtype,
	int& cmdnum, int& tickcount, int& seed, int mouse[2])
{
	if (IsBotClient(client) && GetClientTeam(client) > TEAM_SPECTATOR && IsPlayerAlive(client)) {
		if (bot_can_think[client]) {
			bool is_stuck = CheckIfStuck(client);
			bool has_active_plan = HasActivePlan(client);
			
			//PrintToServer("%d THINKING (is_stuck: %d, has_active_plan: %d)",
			//	client,
			//	is_stuck,
			//	has_active_plan
			//	);
			
			float next_angles[3];
			if (has_active_plan) {
				buttons |= DecideAngles(client, is_stuck, next_angles);
				if (g_cMaxSpinSpeed.IntValue != 0) {
					VectorLerp(prev_angles[client], next_angles, angles, g_cMaxSpinSpeed.FloatValue);
				}
			}
			
			prev_angles[client] = angles;
			
			if (IsBotThinkFrame(client)) {
				buttons |= g_cBitsOverride.IntValue;
				
				if (has_active_plan) {
					buttons |= DecideVelocity(client, is_stuck, angles, vel);
				}
				
				prev_vel[client] = vel;
			}
			else {
				vel = prev_vel[client];
				
				// Keep ducking if that was what we were doing on last think
				if (prev_buttons[client] & IN_DUCK) {
					buttons |= IN_DUCK;
				}
			}
		}
		
		if (buttons & IN_ATTACK) {
			buttons &= ~(IN_SPRINT | IN_RUN);
		}
		
		prev_buttons[client] = buttons;
	}
	
	return Plugin_Continue;
}

public void OnRoundStart(Handle event, const char[] name, bool dontBroadcast)
{
	RoundStartActions();
}

void RoundStartActions()
{
	round_start_time = GetGameTime();
	
	for (int client = 1; client <= MaxClients; ++client) {
		_enemy_last_spotted_time_single[client] = 0.0;
		pre_health[client] = 100;
		bot_can_think[client] = false;
		bot_last_took_dmg_from[client] = 0;
		
		for (int other_client = 1; other_client <= MaxClients; ++other_client) {
			_enemy_positions[client][other_client] = vec3_origin;
			_enemy_last_spotted_time[client][other_client] = 0.0;
		}
	}
	
	SetRandomSeed(GetGameTickCount());
	g_cBotClass.SetInt(GetRandomInt(CLASS_RECON, CLASS_SUPPORT), true, false);
	
	if (g_hGenericTimer != null) {
		delete g_hGenericTimer;
		g_hGenericTimer = null;
	}
	g_hGenericTimer = CreateTimer(BOT_TIMED_THINK_INTERVAL, Timer_Generic, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
}

public void OnPlayerSpawn(Event event, const char[] name, bool dontBroadcast)
{
	int userid = event.GetInt("userid");
	int client = GetClientOfUserId(userid);
	
	if (client != 0 && IsBotClient(client)) {
		pre_health[client] = 100;
		bot_can_think[client] = false;
		
		SetRandomSeed(GetGameTickCount() + client);
		_close_enough_dist[client] = GetRandomInt(BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE_MIN, BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE_MAX);
		bot_last_took_dmg_from[client] = 0;
		
		// Just making sure we aren't stuck holding keys.
		FakeClientCommand(client, "-attack;-jump;-duck;-sprint;joinstar 1");
		// Join alpha squad!
		SetPlayerStar(client, 1);
		
		CreateTimer(0.1, Timer_FreeToThink, userid, TIMER_FLAG_NO_MAPCHANGE);
		CreateTimer(1.0, Timer_ChooseWeapons, userid, TIMER_FLAG_NO_MAPCHANGE);
	}
}

public Action OnPlayerHurt(Event event, const char[] name, bool dontBroadcast)
{
	int victim = GetClientOfUserId(event.GetInt("userid"));
	
	if (victim != 0 || IsBotClient(victim)) {
		bot_last_took_dmg_from[victim] = GetClientOfUserId(event.GetInt("attacker"));
	}
	
	return Plugin_Continue;
}

public Action Timer_ChooseWeapons(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	if (client != 0) {
		StripPlayerWeapons(client);
		
		int player_class = GetPlayerClass(client);
		
		char weapon_name[18 + 1];
		SuggestWeapon(player_class, GetPlayerXP(client), weapon_name, sizeof(weapon_name));
		
		//PrintToChatAll("Class %s: chose weapon %s at XP %d",
		//	(player_class == CLASS_RECON) ? "Recon" : (player_class == CLASS_ASSAULT) ? "Assault" : "Support",
		//	weapon_name,
		//	GetPlayerXP(client));
		
		GivePlayerItem(client, weapon_name);
		GivePlayerItem(client, (player_class == CLASS_RECON) ? "weapon_milso" : (player_class == CLASS_ASSAULT) ? "weapon_tachi" : "weapon_kyla");
	}
	return Plugin_Stop;
}

public Action Timer_FreeToThink(Handle timer, int userid)
{
	int client = GetClientOfUserId(userid);
	if (client != 0) {
		bot_can_think[client] = true;
		MakePlan(client);
	}
	return Plugin_Stop;
}
