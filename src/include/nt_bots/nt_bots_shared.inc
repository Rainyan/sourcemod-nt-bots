#if defined(_NT_BOTS_SHARED_INC_)
#endinput
#endif
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.5"

#define BOTS_MSG_PREFIX "[BOT]"

#define NEO_MAX_PLAYERS 32

#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE 64

#define BOT_TIMED_THINK_INTERVAL 1.0

#define MAX_AREA_MOVES_PLANNED_FOR_FUTURE 100

const SDKHookType thinkHookType = SDKHook_PreThink;

int prev_buttons[NEO_MAX_PLAYERS + 1];

int pre_health[NEO_MAX_PLAYERS + 1];

float target_pos[NEO_MAX_PLAYERS + 1][3];

int bot_clients[NEO_MAX_PLAYERS];
int num_bot_clients;

stock float vec3_origin[3] = { 0.0, 0.0, 0.0 };

stock int ray_color[4] = { 255, 255, 255, 255 };

ConVar g_cBitsOverride = null;
ConVar g_cAimInaccuracy = null;
ConVar g_cReactionTime = null;
ConVar g_cSuppressFireTime = null;
ConVar g_cForgetEnemyTime = null;
ConVar g_cBotAimSpeed = null;
ConVar g_cNoTarget = null;
ConVar g_cMaxSpinSpeed = null;

int rayIndex = 0;

int server_tick_rate;

stock const String:bot_player_models[2][3][] = {
	// Jinrai
	{
		// Recon
		{
			"models/player/jinrai_msf.mdl"
		},
		// Assault
		{
			"models/player/jinrai_mam.mdl"
		},
		// Support
		{
			"models/player/jinrai_mhm.mdl"
		}
	},
	// NSF
	{
		// Recon
		{
			"models/player/nsf_gsf.mdl"
		},
		// Assault
		{
			"models/player/nsf_gam.mdl"
		},
		// Support
		{
			"models/player/nsf_ghm.mdl"
		}
	}
};
