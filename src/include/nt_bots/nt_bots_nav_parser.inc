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
		ThrowError("Format failed");
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
		
		Area area;
		decl String:areaid[16] = "area_0";
		for (int area_i = 0; kv.JumpToKey(areaid, false);
			Format(areaid, sizeof(areaid), "area_%d", ++area_i))
		{
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
				
				decl String:spot_id[16] = "spot_0";
				for (int spot = 0; kv.JumpToKey(spot_id, false); Format(spot_id, sizeof(spot_id), "spot_%d", ++spot)) {
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
			kv.GoBack();
			
			AddArea(area);
			area.Empty();
		}
	}
	
	return true;
}
#endif // _NT_BOTS_NAV_PARSER_INC_