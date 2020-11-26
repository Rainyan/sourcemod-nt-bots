#if !defined(_NT_BOTS_SHARED_INC_)
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.3"

#define NEO_MAX_PLAYERS 32

#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE 128.0

#define BOT_TIMED_THINK_INTERVAL 2.0

#define MAX_AREAS 1024

#define MAX_AREA_MOVES_PLANNED_FOR_FUTURE 100

enum NavAttributeType {
	NAV_MESH_NOFLAGS = 0,
	
	NAV_MESH_CROUCH		= 0x0001, // must crouch to use this node/area
	NAV_MESH_JUMP			= 0x0002, // must jump to traverse this area
	NAV_MESH_PRECISE		= 0x0004, // do not adjust for obstacles, just move along area
	NAV_MESH_NO_JUMP		= 0x0008, // inhibit discontinuity jumping
	NAV_MESH_STOP			= 0x0010, // must stop when entering this area
	NAV_MESH_RUN			= 0x0020, // must run to traverse this area
	NAV_MESH_WALK			= 0x0040, // must walk to traverse this area
	NAV_MESH_AVOID		= 0x0080, // avoid this area unless alternatives are too dangerous
	NAV_MESH_TRANSIENT	= 0x0100, // area may become blocked, and should be periodically checked
	NAV_MESH_DONT_HIDE	= 0x0200, // area should not be considered for hiding spot generation
	NAV_MESH_STAND		= 0x0400, // bots hiding in this area should stand
	NAV_MESH_NO_HOSTAGES	= 0x0800, // hostages shouldn't use this area
};

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