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
	
	g_cBotClass = FindConVar("bot_class");
	if (g_cBotClass == null) {
		SetFailState("Failed to find native cvar bot_class");
	}
}

public void OnConfigsExecuted()
{
	ConnectToDatabase();
}

public void OnMapStart()
{
	UpdateDbTableNamesForThisMap();
	
	rayIndex = PrecacheModel("sprites/purplelaser1.spr", true);
	if (rayIndex == 0) {
		SetFailState("Failed to precache model");
	}
	
	CreateTimer(BOT_TIMED_THINK_INTERVAL, Timer_Generic, _, TIMER_REPEAT | TIMER_FLAG_NO_MAPCHANGE);
	
	for (int client = 1; client <= MaxClients; ++client) {
		pre_health[client] = 100;
		if (IsClientInGame(client) && IsBotClient(client)) {
			BotSetup(client);
		}
	}
}

void BotSetup(int client)
{
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
		
		prev_buttons[client] = buttons;
	}
	
	return Plugin_Continue;
}

public void OnRoundStart(Handle event, const char[] name, bool dontBroadcast)
{
	SetRandomSeed(GetGameTickCount());
	g_cBotClass.SetInt(GetRandomInt(CLASS_RECON, CLASS_SUPPORT), true, false);
	
	for (int client = 1; client <= MaxClients; ++client) {
		for (int other_client = 1; other_client <= MaxClients; ++other_client) {
			_enemy_positions[client][other_client] = vec3_origin;
		}
	}
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
		
		FakeClientCommand(client, "-attack");
		FakeClientCommand(client, "-jump");
		FakeClientCommand(client, "-duck");
		FakeClientCommand(client, "-sprint");
		
		CreateTimer(0.1, Timer_FreeToThink, userid, TIMER_FLAG_NO_MAPCHANGE);
		CreateTimer(1.0, Timer_ChooseWeapons, userid, TIMER_FLAG_NO_MAPCHANGE);
	}
}

public Action OnPlayerHurt(Event event, const char[] name, bool dontBroadcast)
{
	int victim = GetClientOfUserId( event.GetInt("userid"));
	
	if (victim == 0 || !IsBotClient(victim)) {
		return Plugin_Continue;
	}
	
	int health = event.GetInt("health");
	float damage = 1.0 * (pre_health[victim] - health);
	
	// We can't reasonably block a lethal blow without major reworking of this.
	// If this is a lethal hit, just let it pass.
	if (damage <= 0 || damage >= health) {
		return Plugin_Continue;
	}
	
	// TODO: is there some better way to handle this?
	// Because we're hot swapping living bot classes after spawn,
	// the game logic still treats them as recons in terms of damage.
	// So we have to manually adjust damage here.
	switch (GetPlayerClass(victim)) {
		case CLASS_ASSAULT: damage *= 0.833333;
		case CLASS_SUPPORT: damage *= 0.466666;
	}
	
	health = pre_health[victim] - RoundToNearest(damage);
	if (health < 0) {
		health = 0;
	}
	
	event.SetInt("health", health);
	
	pre_health[victim] = health;
	
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
