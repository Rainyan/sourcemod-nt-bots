#if defined(_NT_BOTS_SHARED_INC_)
#endinput
#endif
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.5"

#define BOTS_MSG_PREFIX "[BOT]"

#define NEO_MAX_PLAYERS 32

// Bot will randomly decide their tolerance for waypoint distance from this range.
// This will slightly change their pathing, offering at least some variance in encounters.
#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE_MIN 32
#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE_MAX 64

#define BOT_TIMED_THINK_INTERVAL 10.0

#define MAX_AREA_MOVES_PLANNED_FOR_FUTURE 100

int _close_enough_dist[NEO_MAX_PLAYERS + 1];

enum {
	WEPNAME_GHOST = 0,
	WEPNAME_MPN,
	WEPNAME_SRM,
	WEPNAME_SRM_S,
	WEPNAME_JITTE,
	WEPNAME_JITTESCOPED,
	WEPNAME_ZR68C,
	WEPNAME_ZR68L,
	WEPNAME_ZR68S,
	WEPNAME_SUPA7,
	WEPNAME_M41,
	WEPNAME_M41S,
	WEPNAME_MX,
	WEPNAME_MX_SILENCED,
	WEPNAME_AA13,
	WEPNAME_SRS,
	WEPNAME_PZ
};

float _enemy_positions[NEO_MAX_PLAYERS + 1][NEO_MAX_PLAYERS + 1][3];
float _enemy_last_spotted_time[NEO_MAX_PLAYERS + 1][NEO_MAX_PLAYERS + 1];
float _enemy_last_spotted_time_single[NEO_MAX_PLAYERS + 1];
float round_start_time;

const SDKHookType thinkHookType = SDKHook_PreThink;

int prev_buttons[NEO_MAX_PLAYERS + 1];

int pre_health[NEO_MAX_PLAYERS + 1];

int bot_clients[NEO_MAX_PLAYERS];
int num_bot_clients;

int bot_last_took_dmg_from[NEO_MAX_PLAYERS + 1];

bool bot_can_think[NEO_MAX_PLAYERS + 1];

stock float vec3_origin[3] = { 0.0, 0.0, 0.0 };

stock int ray_color[4] = { 255, 255, 255, 255 };

Handle g_hGenericTimer = null;

ConVar g_cBitsOverride = null;
ConVar g_cAimInaccuracy = null;
ConVar g_cReactionTime = null;
ConVar g_cSuppressFireTime = null;
ConVar g_cForgetEnemyTime = null;
ConVar g_cBotAimSpeed = null;
ConVar g_cNoTarget = null;
ConVar g_cMaxSpinSpeed = null;
ConVar g_cBotClass = null;
ConVar g_cIsKillConfirmedMode = null;
ConVar g_cDrawDebugBeams = null;

int rayIndex = 0;

int server_tick_rate;
