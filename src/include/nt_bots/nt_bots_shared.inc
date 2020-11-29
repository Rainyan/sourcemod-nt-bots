#if !defined(_NT_BOTS_SHARED_INC_)
#define _NT_BOTS_SHARED_INC_

#define PLUGIN_VERSION "0.3"

#define NEO_MAX_PLAYERS 32

#define BOT_CLOSE_ENOUGH_TO_TARGET_DISTANCE 128.0

#define BOT_TIMED_THINK_INTERVAL 2.0

#define MAX_AREAS 1024
#define MAX_APPROACH_AREAS 16
#define MAX_OVERLAPPING_AREAS MAX_AREAS // TODO: set properly

#define MAX_AREA_MOVES_PLANNED_FOR_FUTURE 100

#define NAV_MESH_CROUCH			(1 << 0)
#define NAV_MESH_JUMP			(1 << 1)
#define NAV_MESH_PRECISE			(1 << 2)
#define NAV_MESH_NO_JUMP			(1 << 3)
#define NAV_MESH_STOP			(1 << 4)
#define NAV_MESH_RUN				(1 << 5)
#define NAV_MESH_WALK			(1 << 6)
#define NAV_MESH_AVOID			(1 << 7)
#define NAV_MESH_TRANSIENT		(1 << 8)
#define NAV_MESH_DONT_HIDE		(1 << 9)
#define NAV_MESH_STAND			(1 << 10)
#define NAV_MESH_NO_HOSTAGES	(1 << 11)

#define UNDEFINED_PLACE 0 // ie: "no place"
#define ANY_PLACE 0xFFFF

// Team indices for arrays where only 2 teams are required
// (Jinrai and NSF), so TEAM_SPECTATOR etc. don't need to be allocated.
// Use the teamidx_to_navteamidx(...) and navteamidx_to_teamidx(...)
// helpers to convert between these enums and actual engine team indices.
enum {
	NAV_TEAM_IDX_JINRAI = 0,
	NAV_TEAM_IDX_NSF,
	
	NUM_NAV_TEAMS
};

enum NavDirType
{
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};
new const String:nav_dir_names[NUM_DIRECTIONS][] = { "north", "east", "south", "west" };

enum NavTraverseType {
	// NOTE: First 4 directions MUST match NavDirType
	GO_NORTH = 0,
	GO_EAST,
	GO_SOUTH,
	GO_WEST,
	GO_LADDER_UP,
	GO_LADDER_DOWN,
	GO_JUMP,

	NUM_TRAVERSE_TYPES
};

const SDKHookType thinkHookType = SDKHook_PreThink;

float vec3_origin[3] = { 0.0, 0.0, 0.0 };

float s_targetPos[3];

int s_movementPlan[MAX_AREA_MOVES_PLANNED_FOR_FUTURE];
int s_movementPlanStack = -1;

int s_master_marker;
int s_open_list_id;

float s_ang[3];

ConVar g_cAngleOffset = null;
ConVar g_cBitsOverride = null;

int rayIndex = 0;

#endif // _NT_BOTS_SHARED_INC_