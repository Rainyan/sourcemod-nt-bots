#if !defined(_NT_BOTS_NAV_AREA_INC_)
#define _NT_BOTS_NAV_AREA_INC_

#define MAX_AREA_CONNECTIONS 9

enum struct NavArea {
	int id; // unique id of this area
	int attribute_flags; // my NavTraverseType bits
	
	int parent_id; // the area just prior to this on in the search path
	NavTraverseType parent_how; // how we get from parent area to us
	
	int marker; // used to flag the area as visited
	int open_marker; // if this equals the current marker value, we are on the open list
	
	int prev_open_area_id; // only valid if this.IsOpen()
	int next_open_area_id; // only valid if this.IsOpen()
	
	float extents_nw_corner[3];
	float implicit_height_nw;
	
	float extents_se_corner[3];
	float implicit_height_se;
	
	int connected_areas_north[MAX_AREA_CONNECTIONS];
	int connected_areas_east[MAX_AREA_CONNECTIONS];
	int connected_areas_south[MAX_AREA_CONNECTIONS];
	int connected_areas_west[MAX_AREA_CONNECTIONS];
	
	int num_connected_areas[NUM_DIRECTIONS];
	
	bool is_blocked;
	bool is_underwater;
	
	float danger[NUM_NAV_TEAMS]; // decaying value: based on casualties in this area, how dangerous is it
	float danger_timestamp[NUM_NAV_TEAMS]; // timestamp of last danger update
	
	float cleared_timestamp[NUM_NAV_TEAMS]; // when this area was last cleared of enemies
	
	// --------------------------------------------------------------------------------
	// Approach areas start
	// --------------------------------------------------------------------------------
	// We can't have multidimensional enum struct fields ("ApproachInfo[MAX_APPROACH_AREAS]"),
	// so exposing that struct abstraction here, instead, to work around syntax limitations.
	//
	// (The "NavConnect" referred below is a union of (area id/*area); representing it as area id here.)
	//
	// FIXME/TODO: populate this data from the parser (it's available in the kv's)
	// --------------------------------------------------------------------------------
	int approach_info_navconnect_here[MAX_APPROACH_AREAS]; // the approach area
	int approach_info_navconnect_prev[MAX_APPROACH_AREAS]; // the area just before the approach area on the path
	int approach_info_navconnect_next[MAX_APPROACH_AREAS]; // the area just after the approach area on the path
	NavTraverseType approach_info_prev_to_here_how[MAX_APPROACH_AREAS];
	NavTraverseType approach_info_here_to_next_how[MAX_APPROACH_AREAS];
	int num_approach_areas;
	
	// Accessor for a single "ApproachInfo", at index i. Passes that data by reference.
	// Returns a success boolean.
	// Won't succeed if requesting an out-of-bounds index.
	// Won't succeed if requesting an index higher than current num_approach_areas.
	bool GetApproachInfo(const int i, int& here, int& prev, int& next,
		NavTraverseType& prev_to_here_how, NavTraverseType& here_to_next_how)
	{
		if (i < 0 || i >= MAX_APPROACH_AREAS) {
			return false;
		} else if (i >= this.num_approach_areas) {
			return false;
		}
		here = this.approach_info_navconnect_here[i];
		prev = this.approach_info_navconnect_prev[i];
		next = this.approach_info_navconnect_next[i];
		prev_to_here_how = this.approach_info_prev_to_here_how[i];
		here_to_next_how = this.approach_info_here_to_next_how[i];
		return true;
	}
	// --------------------------------------------------------------------------------
	// Approach areas end
	// --------------------------------------------------------------------------------
	
	void Initialize()
	{
		this.marker = 0;
		this.parent_id = 0;
		this.parent_how = GO_NORTH;
		this.attribute_flags = 0;
		this.is_blocked = false;
		this.is_underwater = false;
		
		for (int i = 0; i < NUM_NAV_TEAMS; ++i) {
			this.danger[i] = 0.0;
			this.danger_timestamp[i] = 0.0;
			this.cleared_timestamp[i] = 0.0;
		}
		
		this.num_approach_areas = 0;
	}
	
	void SetParent(const int parent_id, const NavTraverseType parent_how = NUM_TRAVERSE_TYPES)
	{
		this.parent_id = parent_id;
		this.parent_how = parent_how;
	}
	
	void GetOrigin(float out[3])
	{
		AddVectors(this.extents_nw_corner, this.extents_se_corner, out);
		ScaleVector(out, 0.5);
	}
	
	void MakeNewMarker()
	{
		if (++s_master_marker == 0) {
			s_master_marker = 1;
		}
	}
	
	void Mark()
	{
		this.marker = s_master_marker;
	}
	
	bool IsMarked()
	{
		return this.marker == s_master_marker;
	}
	
	void ChangeMasterMarker()
	{
		++s_master_marker;
	}
	
	// Decay the danger values
	void DecayDanger()
	{
		// one kill == 1.0, which we will forget about in two minutes (1.0 / 120.0)
#define DECAY_RATE 0.008333
		for (int i = 0; i< NUM_NAV_TEAMS; ++i) {
			// Only bother with danger decay if the danger isn't already zeroed.
			if (this.danger[i] != 0) {
				float delta_time = GetGameTime() - this.danger_timestamp[i];
				this.danger[i] -= DECAY_RATE * delta_time;
				if (this.danger[i] < 0) {
					this.danger[i] = 0.0;
				}
			}
			this.danger_timestamp[i] = GetGameTime();
		}
	}
	
	bool IsOpen()
	{
		return this.open_marker == s_master_marker;
	}
	
#if(0)
	// TODO: Might be unnecessary
	void CopyTo(NavArea[1] other)
	{
		other[0].id = this.id;
		other[0].parent_id = this.parent_id;
		other[0].attribute_flags = this.attribute_flags;
		
		other[0].extents_nw_corner = this.extents_nw_corner;
		other[0].extents_se_corner = this.extents_se_corner;
		
		other[0].connected_areas_north = this.connected_areas_north;
		other[0].connected_areas_east = this.connected_areas_east;
		other[0].connected_areas_south = this.connected_areas_south;
		other[0].connected_areas_west = this.connected_areas_west;
		
		other[0].num_connected_areas = this.num_connected_areas;
	}
#endif
	void Empty()
	{
		this.id = 0;
		this.parent_id = 0;
		this.attribute_flags = 0;
		
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
		
		for (NavDirType dir = NORTH; dir < NUM_DIRECTIONS; ++dir) {
			this.num_connected_areas[dir] = 0;
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

#endif // _NT_BOTS_NAV_AREA_INC_