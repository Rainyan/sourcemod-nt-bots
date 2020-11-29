#if !defined(_NT_BOTS_NAV_PARSER_INC_)
#define _NT_BOTS_NAV_PARSER_INC_

bool ParseNavOfMap(const char[] mapName)
{
	decl String:navPath[PLATFORM_MAX_PATH];
	decl String:bspPath[PLATFORM_MAX_PATH];
	
	if (Format(navPath, sizeof(navPath), "maps/%s.kv", mapName) < 1) {
		ThrowError("Format failed");
	} else if (!FileExists(navPath)) {
		SetFailState("File does not exist: %s", navPath);
		return false;
	}
	
	if (Format(bspPath, sizeof(bspPath), "maps/%s.bsp", mapName) < 1) {
		SetFailState("Format failed");
	} else if (!FileExists(bspPath)) {
		SetFailState("File does not exist: %s", bspPath);
		return false;
	}
	
	KeyValues kv = new KeyValues("nav");
	if (!kv.ImportFromFile(navPath)) {
		SetFailState("Failed to import map \"%s\" nav file: \"%s\"", mapName, navPath);
	}
	
	bool success = ParseNavFile(kv, bspPath);
	delete kv;
	return success;
}

// Parse based on valid file handle.
// Caller is responsible for the KeyValues handle memory management.
bool ParseNavFile(const KeyValues& kv, const char[] bspPath)
{
	decl String:section[32];
	
	// meta
	if (kv.GotoFirstSubKey()) {
		if (kv.GetNum("nav_parse_successful", 0) != 1) {
			return false;
		}
	} else {
		return false;
	}
	
	// header
	if (!kv.GotoNextKey()) {
		SetFailState("GotoNextKey failed for header");
	} else {
		if (!kv.GetSectionName(section, sizeof(section))) {
			SetFailState("HEADER sectionname");
			return false;
		}
		
		//PrintToServer("Processing section: %s", section);
		
		// TODO: use the actual hexadecimal representation
		decl String:magic[9];
		if (!kv.GetString("magic", magic, sizeof(magic), "\0")) {
			SetFailState("Magic 1");
			return false;
		} else if (!StrEqual(magic, "FEEDFACE", false)) {
			SetFailState("Magic 2");
			return false;
		}
		
		if (kv.GetNum("version", 0) != 9) { // TODO: add variable for format version
			SetFailState("Version mismatch");
			return false;
		}
		
		int nav_reported_bsp_size = kv.GetNum("bsp_size", 0);
		if (nav_reported_bsp_size != FileSize(bspPath)) {
			SetFailState("nav_reported_bsp_size mismatch (%d vs %d)",
				nav_reported_bsp_size, FileSize(bspPath));
			return false;
		}
	}
	
	// places
	if (!kv.GotoNextKey()) {
		SetFailState("GotoNextKey failed for places");
	}
	
	// navigation_areas
	if (!kv.GotoNextKey()) {
		SetFailState("GotoNextKey failed for navigation_areas");
	} else {
		if (!kv.GetSectionName(section, sizeof(section))) {
			return false;
		}
		
		//PrintToServer("Processing section: %s", section);
		
		float invalid[3] = { -123.4, -1337.0, -432.1 };
		
		decl String:areaid[16] = "area_0";
		for (int area_i = 0; kv.JumpToKey(areaid, false);
			Format(areaid, sizeof(areaid), "area_%d", ++area_i))
		{
			NavArea area;
			area.Initialize();
			
			area.id = kv.GetNum("id", -1);
			if (area.id == -1) {
				SetFailState("%s :: %s: Failed to get \"id\"", section, areaid);
			}
			
			//PrintToServer("%s: id = %d", areaid, area.id);
			
			{
				kv.JumpToKey("extents_nw_corner", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				kv.GetVector("origin", area.extents_nw_corner, invalid);
				if (VectorsEqual(area.extents_nw_corner, invalid)) {
					SetFailState("Invalid vector at area.extents_nw_corner");
				}
				
				//PrintToServer("Vector: %f %f %f", area.extents_nw_corner[0], area.extents_nw_corner[1], area.extents_nw_corner[2]);
				
				area.implicit_height_nw = kv.GetFloat("implicit_height", -1.0);
				//PrintToServer("Implicit height: %f", area.implicit_height_nw);
				
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("extents_se_corner", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				kv.GetVector("origin", area.extents_se_corner, invalid);
				if (VectorsEqual(area.extents_se_corner, invalid)) {
					SetFailState("Invalid vector at area.extents_se_corner");
				}
				
				area.implicit_height_se = kv.GetFloat("implicit_height", -1.0);
				//PrintToServer("Implicit height: %f", area.implicit_height_se);
				
				kv.GoBack();
			}
			
			// Set extents XYZ
			for (int i = 0; i < 2; ++i) {
				if (area.extents_nw_corner[i] < area.extents_se_corner[i]) {
					area.extent.lo[i] = area.extents_nw_corner[i];
					area.extent.hi[i] = area.extents_se_corner[i];
				} else {
					area.extent.lo[i] = area.extents_se_corner[i];
					area.extent.hi[i] = area.extents_nw_corner[i];
				}
			}
			area.extent.lo[2] = area.extents_nw_corner[2];
			area.extent.hi[2] = area.extents_nw_corner[2];
			
			area.ne_z = area.extents_nw_corner[2];
			area.sw_z = area.extents_se_corner[2];
			
			// Calculate the centroid of this area
			AddVectors(area.extents_nw_corner, area.extents_se_corner, area.centroid);
			ScaleVector(area.centroid, 0.5);
			
			{
				kv.JumpToKey("dirs", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				for (int dir = 0; dir < sizeof(dirnames); ++dir) {
					if (!kv.JumpToKey(dirnames[dir], false)) {
						SetFailState("Failed to jump to dir %s", dirnames[dir]);
						return false;
					}
					decl String:connection_id[16] = "connection_0";
					for (int connection = 0; kv.JumpToKey(connection_id, false); Format(connection_id, sizeof(connection_id), "connection_%d", ++connection)) {
						int new_connection = kv.GetNum("connects_to_area_id", -1);
						if (new_connection == -1) {
							SetFailState("Invalid connection id");
						}
						
						if (area.num_connected_areas[dir] > MAX_AREA_CONNECTIONS) {
							decl String:sectionname[64];
							kv.GetSectionName(sectionname, sizeof(sectionname));
							SetFailState("area.num_connected_areas (id %d) overflowed in area %d::%s (section name: \"%s\" --> connects_to_area_id %d)",
								area.id, area.num_connected_areas[dir], dirnames[dir], sectionname, new_connection);
						}
						switch (dir)
						{
							case NORTH:	{ area.connected_areas_north[area.num_connected_areas[dir]] = new_connection; }
							case EAST:	{ area.connected_areas_east[area.num_connected_areas[dir]] = new_connection; }
							case SOUTH:	{ area.connected_areas_south[area.num_connected_areas[dir]] = new_connection; }
							case WEST:	{ area.connected_areas_west[area.num_connected_areas[dir]] = new_connection; }
							default:			{ SetFailState("Unknown direction %d", dir); }
						}
						area.num_connected_areas[dir] += 1;
						
						//PrintToServer("area id %d has %sern connection to area id %d", area.id, dirnames[dir], new_connection);
						kv.GoBack();
					}
					kv.GoBack();
				}
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("hiding_spots", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				decl String:hiding_spot_id[16] = "spot_0";
				for (int spot = 0; kv.JumpToKey(hiding_spot_id, false); Format(hiding_spot_id, sizeof(hiding_spot_id), "spot_%d", ++spot)) {
					//int hiding_spot_id = kv.GetNum("id", -1);
					//int flags = kv.GetNum("flags", -1);
					
					kv.JumpToKey("pos", false);
					//float hiding_spot_origin[3];
					//kv.GetVector("origin", hiding_spot_origin);
					kv.GoBack();
					
					//PrintToServer("area id %d has hiding spot id %d (pos: %f %f %f), flags: %d",
					//	area.id, hiding_spot_id,
					//	hiding_spot_origin[0], hiding_spot_origin[1], hiding_spot_origin[2],
					//	flags);
					
					kv.GoBack();
				}
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("approach_areas", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				decl String:approach_area_id[16] = "area_0";
				for (int approach_area = 0; kv.JumpToKey(approach_area_id, false);
					Format(approach_area_id, sizeof(approach_area_id), "area_%d", ++approach_area))
				{
					if (approach_area >= MAX_APPROACH_AREAS) {
						SetFailState("Approach areas overflowed");
					}
					
					int this_area_id = kv.GetNum("this_area_id", -1);
					int prev_area_id = kv.GetNum("prev_area_id", -1);
					int raw_prev_area_to_here_how_type = kv.GetNum("prev_area_to_here_how_type", -1);
					int next_area_id = kv.GetNum("next_area_id", -1);
					int raw_here_to_next_area_how_type = kv.GetNum("here_to_next_area_how_type", -1);
					
					if (raw_prev_area_to_here_how_type < 0 || raw_prev_area_to_here_how_type >= view_as<int>(NUM_TRAVERSE_TYPES)) {
						SetFailState("Got invalid NavTraverseType raw_prev_area_to_here_how_type: %d",
							view_as<int>(raw_prev_area_to_here_how_type));
					}
					if (raw_here_to_next_area_how_type < 0 || raw_here_to_next_area_how_type >= view_as<int>(NUM_TRAVERSE_TYPES)) {
						SetFailState("Got invalid NavTraverseType raw_here_to_next_area_how_type: %d",
							view_as<int>(raw_here_to_next_area_how_type));
					}
					
					area.approach_info_navconnect_here[approach_area] = this_area_id;
					area.approach_info_navconnect_prev[approach_area] = prev_area_id;
					area.approach_info_navconnect_next[approach_area] = next_area_id;
					area.approach_info_prev_to_here_how[approach_area] = view_as<NavTraverseType>(raw_prev_area_to_here_how_type);
					area.approach_info_here_to_next_how[approach_area] = view_as<NavTraverseType>(raw_here_to_next_area_how_type);
					
					kv.GoBack();
				}
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("encounter_spots", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				//PrintToServer("Section is currently: %s", section);
				
				decl String:encounter_spot_id[16] = "spot_0";
				for (int spot = 0; kv.JumpToKey(encounter_spot_id, false);
					Format(encounter_spot_id, sizeof(encounter_spot_id), "spot_%d", ++spot))
				{
					if (spot >= MAX_ENCOUNTER_SPOTS) {
						SetFailState("Encounter spots overflowed");
					}
					
					int from_area_id = kv.GetNum("from_area_id", -1);
					int from_dir = kv.GetNum("from_dir", -1);
					int to_area_id = kv.GetNum("to_area_id", -1);
					int to_dir = kv.GetNum("to_dir", -1);
					
					if (from_dir < 0 || from_dir >= view_as<int>(NUM_DIRECTIONS)) {
						SetFailState("Invalid from_dir: %d", from_dir);
					} else if (to_dir < 0 || to_dir >= view_as<int>(NUM_DIRECTIONS)) {
						SetFailState("Invalid to_dir: %d", to_dir);
					}
					
					area.encounter_spot_navconnect_from[spot] = from_area_id;
					area.encounter_spot_navconnect_to[spot] = to_area_id;
					area.encounter_spot_from_dir[spot] = view_as<NavDirType>(from_dir);
					area.encounter_spot_to_dir[spot] = view_as<NavDirType>(to_dir);
					
					// TODO: Actually implement hiding spots
					//decl String:hiding_spots_along_this_path_id[16] = "spot_0";
					//for (int spot_along = 0; kv.JumpToKey(hiding_spots_along_this_path_id, false);
					//	Format(hiding_spots_along_this_path_id,
					//	sizeof(hiding_spots_along_this_path_id), "spot_%d", ++spot_along))
					//{
					//	int id = kv.GetNum("id", -1); // spot ID for save/load
					//	int t = kv.GetNum("t", -1); // parametric distance along ray where this spot first has LOS to our path
					//	
					//	kv.GoBack();
					//}
					
					kv.GoBack();
				}
			}
			
			kv.GoBack();
			
			AddArea(area);
		}
	}
	
	if (!BuildOverlapList()) {
		SetFailState("Failed to build overlap list");
	}
	
	return true;
}
#endif // _NT_BOTS_NAV_PARSER_INC_