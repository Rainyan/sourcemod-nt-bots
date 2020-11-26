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
	
	int place; // A place is a named group of navigation areas
	
	float extents_nw_corner[3];
	float implicit_height_nw;
	
	float extents_se_corner[3];
	float implicit_height_se;
	
	int connected_areas_north[MAX_AREA_CONNECTIONS];
	int connected_areas_east[MAX_AREA_CONNECTIONS];
	int connected_areas_south[MAX_AREA_CONNECTIONS];
	int connected_areas_west[MAX_AREA_CONNECTIONS];
	
	int num_connected_areas[NUM_DIRECTIONS];
	
	void Initialize()
	{
		this.marker = 0;
		this.parent_id = 0;
		this.parent_how = GO_NORTH;
		this.attribute_flags = 0;
		
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