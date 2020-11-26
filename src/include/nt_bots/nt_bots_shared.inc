#if !defined(_NT_BOTS_SHARED_INC_)
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.3"

#define NEO_MAX_PLAYERS 32

#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE 128.0

#define BOT_TIMED_THINK_INTERVAL 2.0

#define MAX_AREAS 1024

#define MAX_AREA_MOVES_PLANNED_FOR_FUTURE 100

const SDKHookType thinkHookType = SDKHook_PreThink;

float vec3_origin[3] = { 0.0, 0.0, 0.0 };

float s_targetPos[3];

int s_movementPlan[MAX_AREA_MOVES_PLANNED_FOR_FUTURE];
int s_movementPlanStack = -1;

float s_ang[3];

ConVar g_cAngleOffset = null;
ConVar g_cBitsOverride = null;

int rayIndex = 0;

#endif // _NT_BOTS_SHARED_INC_