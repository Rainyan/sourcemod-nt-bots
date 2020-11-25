#if !defined(_NT_BOTS_BASE_INC_)
#define _NT_BOTS_BASE_INC_

#define MAX_AREA_CONNECTIONS 9
#define MAX_AREAS 1024

enum Direction {
	DIR_NORTH = 0,
	DIR_EAST,
	DIR_SOUTH,
	DIR_WEST,
	
	NUM_DIRECTIONS
}

enum struct Area {
	int id;
	int attribute_flags;
	
	float extents_nw_corner[3];
	float implicit_height_nw;
	
	float extents_se_corner[3];
	float implicit_height_se;
	
	int connected_areas_north[MAX_AREA_CONNECTIONS];
	int connected_areas_east[MAX_AREA_CONNECTIONS];
	int connected_areas_south[MAX_AREA_CONNECTIONS];
	int connected_areas_west[MAX_AREA_CONNECTIONS];
	
	int num_connected_areas[NUM_DIRECTIONS];
	int area_traversal_stack_head[NUM_DIRECTIONS];
	
	void GetOrigin(float out[3])
	{
		AddVectors(this.extents_nw_corner, this.extents_se_corner, out);
		ScaleVector(out, 0.5);
	}
	
	// TODO: Might be unnecessary
	void Copy(Area[1] other)
	{
		other[0].id = this.id;
		other[0].attribute_flags = this.attribute_flags;
		
		other[0].extents_nw_corner = this.extents_nw_corner;
		other[0].extents_se_corner = this.extents_se_corner;
		
		other[0].connected_areas_north = this.connected_areas_north;
		other[0].connected_areas_east = this.connected_areas_east;
		other[0].connected_areas_south = this.connected_areas_south;
		other[0].connected_areas_west = this.connected_areas_west;
		
		other[0].num_connected_areas = this.num_connected_areas;
		other[0].area_traversal_stack_head = this.area_traversal_stack_head;
	}
}

static ArrayList _areas = null;
static int _num_areas = 0;

void InitialiseAreasArray()
{
	// Note: ArrayList is not default initialized
	_areas = new ArrayList(sizeof(Area), MAX_AREAS);
	
#if(0) // Test ArrayList iteration
	for (int i = 0; i < MAX_AREAS; ++i) {
		Area dummy;
		dummy.id = i;
		_areas.PushArray(dummy);
	}
	Area placeholder;
	PrintToServer("_areas length: %d, blocksize: %d", _areas.Length, _areas.BlockSize);
	int num_total = 0;
	for (int i = _areas.Length - 1; i >= _areas.Length - MAX_AREAS; --i) {
		_areas.GetArray(i, placeholder);
		PrintToServer("Found Area at idx: %d (id value: %d)", i, placeholder.id);
		++num_total;
	}
	PrintToServer("Total: %d", num_total);
#endif
}

void ListAreas()
{
	Area area;
	int num_iterated = 0;
	for (int i = _areas.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		_areas.GetArray(i, area);
		PrintToServer("= = = = = = = = = = = = = = = = = = = =\nFound Area at idx: %d (id value: %d)", i, area.id);
		float origin[3];
		area.GetOrigin(origin);
		PrintToServer("XY bounds: [ NW %f %f -  SE %f %f ], and origin: %f %f %f)",
			area.extents_nw_corner[0], area.extents_nw_corner[1],
			area.extents_se_corner[0], area.extents_se_corner[1],
			origin[0], origin[1], origin[2]);
		
		new const String:dirnames[][] = { "north", "east", "south", "west" };
		for (Direction dir = DIR_NORTH; dir < NUM_DIRECTIONS; ++dir) {
			for (int j = 0; j < area.num_connected_areas[dir]; ++j) {
				if (j == 0) {
					PrintToServer("\n\t- Area %d has %d %sern connections total.", area.id, area.num_connected_areas[dir], dirnames[dir]);
				}
				
				int connection = -1;
				switch (dir)
				{
					case DIR_NORTH: { connection = area.connected_areas_north[j]; }
					case DIR_EAST: { connection = area.connected_areas_east[j]; }
					case DIR_SOUTH: { connection = area.connected_areas_south[j]; }
					case DIR_WEST: { connection = area.connected_areas_west[j]; }
				}
				PrintToServer("\t-- Area %d has %sern connection: %d", area.id, dirnames[dir], connection);
			}
		}
	}
}

void AddArea(const Area area)
{
	_areas.PushArray(area);
	++_num_areas;
}

int GetNumAreas()
{
	return _num_areas;
}

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
bool IsBotClient(int client)
{
    return (IsFakeClient(client) && !IsClientSourceTV(client));
}

// Predicate for client position initiated ray trace.
bool DidNotHitSelf(int self, int contentsMask, int target)
{
    return self != target;
}

void MultiplyVectors(float v1[3], float v2[3], float res[3])
{
    res[0] = v1[0] * v2[0];
    res[1] = v1[1] * v2[1];
    res[2] = v1[2] * v2[2];
}

bool VectorsEqual(float v1[3], float v2[3])
{
	return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
}

float Lerp(float a, float b, float scale = 0.0)
{
    if (scale == 0) {
        scale = GetGameFrameTime();
    }
    return a + (b - a) * scale;
}

#endif // _NT_BOTS_BASE_INC_