#if !defined(_NT_BOTS_NAV_MESH_INC_)
#define _NT_BOTS_NAV_MESH_INC_

// The NavMesh is the global interface to the Navigation Mesh.
enum struct NavMesh {
	bool is_loaded; // true if a Navigation Mesh has been loaded
	bool is_from_current_map; // true if the Navigation Mesh was last saved with the current map
	
	void Initialize()
	{
		TheNavAreaList = new ArrayList(sizeof(NavArea), MAX_AREAS);
#if(0) // Test ArrayList iteration
		for (int i = 0; i < MAX_AREAS; ++i) {
			NavArea dummy;
			dummy.id = i;
			TheNavAreaList.PushArray(dummy);
		}
		NavArea placeholder;
		PrintToServer("TheNavAreaList length: %d, blocksize: %d", TheNavAreaList.Length, TheNavAreaList.BlockSize);
		int num_total = 0;
		for (int i = TheNavAreaList.Length - 1; i >= TheNavAreaList.Length - MAX_AREAS; --i) {
			TheNavAreaList.GetArray(i, placeholder);
			PrintToServer("Found NavArea at idx: %d (id value: %d)", i, placeholder.id);
			++num_total;
		}
		PrintToServer("Total: %d", num_total);
		this.DestroyNavigationMesh();
#endif
	}
	
	void DestroyNavigationMesh()
	{
		TheNavAreaList.Clear();
		ClearNumAreasInNavList();
	}
	
	bool LoadCurrentMap()
	{
		this.DestroyNavigationMesh();
		
		char mapName[PLATFORM_MAX_PATH];
		if (GetCurrentMap(mapName, sizeof(mapName)) < 1) {
			SetFailState("Failed to get map name");
		}
		
		bool nav_parse_successful = ParseNavOfMap(mapName);
		
		this.is_loaded = nav_parse_successful;
		this.is_from_current_map = this.is_loaded;
		
		return this.is_from_current_map;
	}
}

#endif // _NT_BOTS_NAV_MESH_INC_