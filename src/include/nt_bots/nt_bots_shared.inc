#if !defined(_NT_BOTS_SHARED_INC_)
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.3"

#define NEO_MAX_PLAYERS 32

#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE 32.0

const SDKHookType thinkHookType = SDKHook_PreThink;

float s_targetPos[3] = { 0.0, 0.0, 0.0 };
float s_ang[3];

ConVar g_cAngleOffset = null;
ConVar g_cBitsOverride = null;

int rayIndex = 0;

#endif // _NT_BOTS_SHARED_INC_