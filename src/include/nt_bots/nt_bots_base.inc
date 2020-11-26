#if !defined(_NT_BOTS_BASE_INC_)
#define _NT_BOTS_BASE_INC_

#define MAX_AREA_CONNECTIONS 9

enum Direction {
	DIR_NORTH = 0,
	DIR_EAST,
	DIR_SOUTH,
	DIR_WEST,
	
	NUM_DIRECTIONS
}

enum struct Area {
	int id;
	NavAttributeType attribute_flags;
	
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
	
#if(0)
	// TODO: Might be unnecessary
	void CopyTo(Area[1] other)
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
#endif
	void Empty()
	{
		this.id = 0;
		this.attribute_flags = NAV_MESH_NOFLAGS;
		
		this.extents_nw_corner = vec3_origin;
		this.implicit_height_nw = 0.0;
		this.extents_se_corner = vec3_origin;
		this.implicit_height_se = 0.0;
		
		for (int i = 0; i < MAX_AREA_CONNECTIONS; ++i) {
			this.connected_areas_north[i] = 0;
			this.connected_areas_east[i] = 0;
			this.connected_areas_south[i] = 0;
			this.connected_areas_west[i] = 0;
		}
		
		for (Direction dir = DIR_NORTH; dir < NUM_DIRECTIONS; ++dir) {
			this.num_connected_areas[dir] = 0;
			this.area_traversal_stack_head[dir] = 0;
		}
	}
	
	float DistanceTo(const int id_other)
	{
		return AreaDistance(this.id, id_other);
	}
	
	int NearestArea(const int[] excludes, const int num_excludes)
	{
		return NearestArea(this.id, excludes, num_excludes);
	}
	
	int NearestConnection(const int[] excludes, const int num_excludes)
	{
		return NearestConnection(this.id, excludes, num_excludes);
	}
	
	int SolvePathFor(const int id_other, int[] out_solution_ids, const int maxlen = MAX_AREAS)
	{
		return GetPathSolution(this.id, id_other, out_solution_ids, maxlen);
	}
	
	bool IsNeighbourOf(const int id_other)
	{
		return AreNeighbours(this.id, id_other);
	}
	
	bool ShouldCrouchHere()
	{
		return (this.attribute_flags & NAV_MESH_CROUCH) ? true : false;
	}
	
	bool ShouldJumpHere()
	{
		return (this.attribute_flags & NAV_MESH_JUMP) ? true : false;
	}
}

float AreaDistance(const int id_a, const int id_b)
{
	Area a[1]; Area b[1];
	if (!GetAreaById(id_a, a)) { ThrowError("Failed to get id"); }
	if (!GetAreaById(id_b, b)) { ThrowError("Failed to get id"); }
	
	float pos_a[3]; float pos_b[3];
	a[0].GetOrigin(pos_a);
	b[0].GetOrigin(pos_b);
	
	return GetVectorDistance(pos_a, pos_b);
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

stock void ListAreas()
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

int GetPathSolution(const int start_area_id, const int target_area_id, int[] solution_path_ids,
	const int max_solution_length = MAX_AREAS)
{
	Area start_area[1];
	Area target_area[1];
	
	if (!GetAreaById(start_area_id, start_area)) {
		SetFailState("!GetAreaById start_area_id");
	}
	if (!GetAreaById(target_area_id, target_area)) {
		SetFailState("!GetAreaById target_area_id");
	}
	
	if (start_area[0].id == target_area[0].id) {
		return 0;
	} else if (max_solution_length < 2) {
		ThrowError("Expected 2 to have space for at least 2 nodes");
		return 0;
	}
	
	//float goal_distance = target_area[0].DistanceTo(start_area[0].id);
	
	int bailout = 0;
	bool found_solution = false;
	
	int num_solution_ids = 0;
	
	while (!found_solution) {
		if (bailout++ >= MAX_AREAS) {
			SetFailState("Bail out");
		}
		
		num_solution_ids = 0;
		solution_path_ids[num_solution_ids++] = target_area[0].id;
		
		float prospective_distance = 0.0;
		
		int iter_id = target_area[0].id;
		
		while (!found_solution) {
			Area iter_area[1];
			if (!GetAreaById(iter_id, iter_area)) {
				SetFailState("!GetAreaById: %d", iter_id);
			}
			
			Area connected_area[1];
			if (!GetAreaById(iter_id, connected_area)) {
				SetFailState("!GetAreaById: %d", iter_id);
			}
			
			float distance = iter_area[0].DistanceTo(connected_area[0].id);
			prospective_distance += distance;
			
			for (Direction dir = DIR_NORTH; !found_solution && dir < NUM_DIRECTIONS; ++dir) {
				for (int j = 0; !found_solution && j < connected_area[0].num_connected_areas[dir]; ++j) {
					int connection = 0;
					switch (dir) {
						case DIR_NORTH: { connection = connected_area[0].connected_areas_north[j]; }
						case DIR_EAST: { connection = connected_area[0].connected_areas_east[j]; }
						case DIR_SOUTH: { connection = connected_area[0].connected_areas_south[j]; }
						case DIR_WEST: { connection = connected_area[0].connected_areas_west[j]; }
						default: { SetFailState("Unknown dir: %d", dir); }
					}
					if (connection == 0) {
						SetFailState("Conn == 0");
					}
					if (connection == start_area[0].id) {
						found_solution = true;
					}
				}
			}
			
			//PrintToServer("Was prior to reassign: %d", iter_id);
			iter_id = iter_area[0].NearestConnection(solution_path_ids, num_solution_ids);
			
			// Something went wrong with solving, just return what we've got thus far
			if (iter_id == 0) {
				return num_solution_ids;
			}
			
			//PrintToServer("Iter id is prepared as: %d", iter_id);
			for (int i = 0; i < num_solution_ids; ++i) {
				if (iter_id == solution_path_ids[i]) {
					SetFailState("iter_id (%d) already in solution_path_ids", iter_id);
				}
				//PrintToServer("solution_path_ids %d: %d", i, solution_path_ids[i]);
			}
			solution_path_ids[num_solution_ids++] = iter_id;
		}
	}
	
	if (found_solution) {
		--num_solution_ids;
		if (num_solution_ids <= 0) {
			SetFailState("found_solution == true, but num_solution_ids == 0");
		}
		
		//PrintToServer("Found solution!");
		return num_solution_ids;
	}
	
	return 0;
}

bool AreNeighbours(const int self_id, const int other_id)
{
	Area self_area[1];
	GetAreaById(self_id, self_area);
	
	for (Direction dir = DIR_NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int i = 0; i < self_area[0].num_connected_areas[dir]; ++i) {
			int connection = 0;
			switch (dir) {
				case DIR_NORTH: { connection = self_area[0].connected_areas_north[i]; }
				case DIR_EAST: { connection = self_area[0].connected_areas_east[i]; }
				case DIR_SOUTH: { connection = self_area[0].connected_areas_south[i]; }
				case DIR_WEST: { connection = self_area[0].connected_areas_west[i]; }
				default: { SetFailState("Unknown direction %d", dir); }
			}
			
			if (connection == 0) {
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				SetFailState("Failed to get %sern connection for id %d", dirnames[dir], self_area[0].id);
			}
			
			if (connection == other_id) {
				return true;
			}
		}
	}
	return false;
}

int NearestConnection(const int self_id, const int[] excludes, const int num_excludes)
{
	int nearest_connection = 0;
	float nearest_distance = 0.0;
	
	Area area[1];
	GetAreaById(self_id, area);
		
	for (Direction dir = DIR_NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int i = 0; i < area[0].num_connected_areas[dir]; ++i) {
			int connection = 0;
			switch (dir) {
				case DIR_NORTH: { connection = area[0].connected_areas_north[i]; }
				case DIR_EAST: { connection = area[0].connected_areas_east[i]; }
				case DIR_SOUTH: { connection = area[0].connected_areas_south[i]; }
				case DIR_WEST: { connection = area[0].connected_areas_west[i]; }
				default: { SetFailState("Unknown direction %d", dir); }
			}
			
			if (connection == 0) {
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				SetFailState("Failed to get %sern connection for id %d", dirnames[dir], area[0].id);
			}
			
			//PrintToServer("Found connection: %d");
			
			bool skip_this = false;
			for (int j = 0; j < num_excludes; ++j) {
				if (connection == excludes[j]) {
					skip_this = true;
					break;
				}
			}
			
			if (!skip_this) {
				float distance = area[0].DistanceTo(connection);
				
				if (nearest_connection == 0 || distance < nearest_distance) {
					nearest_connection = connection;
					nearest_distance = distance;
				}
			}
		}
	}
	
	return nearest_connection;
}

int NearestArea(const int self_id, const int[] excludes, const int num_excludes)
{
	int nearest_area = 0;
	float nearest_distance = 0.0;
	
	Area area;
	int num_iterated = 0;
	for (int i = _areas.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		_areas.GetArray(i, area);
		for (int j = 0; j < num_excludes; ++j) {
			if (area.id == excludes[i]) {
				continue;
			}
		}
		float distance = area.DistanceTo(self_id);
		if (nearest_area == 0 || distance < nearest_distance) {
			nearest_area = area.id;
			nearest_distance = distance;
		}
	}
	
	return nearest_area;
}

stock int GetConnections(const Area area, int[] connections, const int max_connections)
{
	int num_connection = 0;
	for (Direction dir = DIR_NORTH; dir < NUM_DIRECTIONS; ++dir) {
		for (int j = 0; j < area.num_connected_areas[dir]; ++j) {
			int connection = -1;
			switch (dir)
			{
				case DIR_NORTH: { connection = area.connected_areas_north[j]; }
				case DIR_EAST: { connection = area.connected_areas_east[j]; }
				case DIR_SOUTH: { connection = area.connected_areas_south[j]; }
				case DIR_WEST: { connection = area.connected_areas_west[j]; }
			}
			connections[num_connection++] = connection;
		}
	}
	return num_connections;
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

bool GetAreaById(const int id, Area[1] out)
{
	Area area;
	int num_iterated = 0;
	for (int i = _areas.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		_areas.GetArray(i, area);
		if (area.id == id) {
			out[0] = area;
			return true;
		}
	}
	return false;
}

bool GetNearestArea(const float pos[3], Area[1] out_area)
{
	float nearest_distance = -1.0;
	
	int num_iterated = 0;
	for (int i = _areas.Length - 1; num_iterated++ < GetNumAreas(); --i) {
		Area this_area;
		_areas.GetArray(i, this_area);
		
		float this_origin[3];
		this_area.GetOrigin(this_origin);
		float this_distance = GetVectorDistance(pos, this_origin);
		
		if (nearest_distance == -1 || this_distance < nearest_distance) {
			out_area[0] = this_area;
			nearest_distance = this_distance;
		}
	}
	return nearest_distance != -1;
}

int GetNearestOfAreaList(const int this_area_id, const int[] area_list, const int num_areas,
	int& nearest_array_index)
{
	Area area[1];
	if (!GetAreaById(this_area_id, area)) {
		SetFailState("!GetAreaById");
	}
	
	int nearest = 0;
	float nearest_distance;
	
	for (int i = 0; i < num_areas; ++i) {
		float distance = area[0].DistanceTo(area_list[i]);
		if (nearest == 0 || distance < nearest_distance) {
			//PrintToServer("%f < %f", distance, nearest_distance);
			nearest = area_list[i];
			nearest_distance = distance;
			nearest_array_index = i;
		} else if (distance >= nearest_distance) {
			//PrintToServer("%f >= %f", distance, nearest_distance);
		}
	}
	
	return nearest;
}

// Is this client a bot that we want to use for bot AI?
// Assumes a valid client index and IsClientConnected == true.
bool IsBotClient(int client)
{
    return client != 0 && IsFakeClient(client) && !IsClientSourceTV(client);
}

// Predicate for client position initiated ray trace.
bool DidNotHitSelf(int self, int contentsMask, int target)
{
    return self != target;
}

void MultiplyVectors(const float v1[3], const float v2[3], float res[3])
{
    res[0] = v1[0] * v2[0];
    res[1] = v1[1] * v2[1];
    res[2] = v1[2] * v2[2];
}

bool VectorsEqual(const float[3] v1, const float[3] v2, const float max_ulps = 0.0)
{
	// Needs to exactly equal.
	if (max_ulps == 0) {
		return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
	}
	// Allow an inaccuracy of size max_ulps.
	else {
		if (FloatAbs(v1[0] - v2[0]) > max_ulps) { return false; }
		if (FloatAbs(v1[1] - v2[1]) > max_ulps) { return false; }
		if (FloatAbs(v1[2] - v2[2]) > max_ulps) { return false; }
		return true;
	}
}

float Lerp(float a, float b, float scale = 0.0)
{
    if (scale == 0) {
        scale = GetGameFrameTime();
    }
    return a + (b - a) * scale;
}

bool GetPlannedPosition(float[3] pos)
{
	if (s_movementPlanStack == -1) {
		return false;
	}
	
	if (s_movementPlan[s_movementPlanStack] == 0) {
		SetFailState("Invalid movement plan id in stack pos %d", s_movementPlanStack);
	}
	
	Area area[1];
	if (!GetAreaById(s_movementPlan[s_movementPlanStack], area)) {
		return false;
	}
	area[0].GetOrigin(pos);
	return true;
}

#endif // _NT_BOTS_BASE_INC_