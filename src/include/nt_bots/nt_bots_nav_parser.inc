#if !defined(_NT_BOTS_NAV_PARSER_INC_)
#define _NT_BOTS_NAV_PARSER_INC_

bool ParseNavOfMap(const char[] mapName)
{
	decl String:navPath[PLATFORM_MAX_PATH];
	decl String:bspPath[PLATFORM_MAX_PATH];
	
	if (Format(navPath, sizeof(navPath), "maps/%s.kv", mapName) < 1) {
		ThrowError("Format failed");
	} else if (!FileExists(navPath)) {
		PrintToServer("File does not exist: %s", navPath);
		return false;
	}
	
	if (Format(bspPath, sizeof(bspPath), "maps/%s.bsp", mapName) < 1) {
		ThrowError("Format failed");
	} else if (!FileExists(bspPath)) {
		PrintToServer("File does not exist: %s", bspPath);
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
		
		PrintToServer("Processing section: %s", section);
		
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
		
		PrintToServer("Processing section: %s", section);
		
		float vec[3];
		float zero[3] = { -1.0, -1.0, -1.0 };
		
		decl String:areaid[16] = "area_0";
		for (int area = 0; kv.JumpToKey(areaid, false); Format(areaid, sizeof(areaid), "area_%d", ++area)) {
			int area_id = kv.GetNum("id", -1);
			if (area_id == -1) {
				SetFailState("%s :: %s: Failed to get \"id\"", section, areaid);
			}
			
			PrintToServer("%s: id = %d", areaid, area);
			
			{
				kv.JumpToKey("extents_nw_corner", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				PrintToServer("Section is currently: %s", section);
				kv.GetVector("origin", vec, zero);
				
				PrintToServer("Vector: %f %f %f", vec[0], vec[1], vec[2]);
				PrintToServer("Implicit height: %f", kv.GetFloat("implicit_height", -1.0));
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("extents_se_corner", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				PrintToServer("Section is currently: %s", section);
				kv.GetVector("origin", vec, zero);
				
				PrintToServer("Vector: %f %f %f", vec[0], vec[1], vec[2]);
				PrintToServer("Implicit height: %f", kv.GetFloat("implicit_height", -1.0));
				kv.GoBack();
			}
			
			{
				kv.JumpToKey("dirs", false);
				if (!kv.GetSectionName(section, sizeof(section))) {
					return false;
				}
				PrintToServer("Section is currently: %s", section);
				
				new const String:dirnames[][] = { "north", "east", "south", "west" };
				for (int dir = 0; dir < sizeof(dirnames); ++dir) {
					if (!kv.JumpToKey(dirnames[dir], false)) {
						SetFailState("Failed to jump to dir %s", dirnames[dir]);
						return false;
					}
					decl String:connection_id[16] = "connection_0";
					for (int connection = 0; kv.JumpToKey(connection_id, false); Format(connection_id, sizeof(connection_id), "connection_%d", ++connection)) {
						PrintToServer("area id %d has %sern connection to area id %d", area_id, dirnames[dir], kv.GetNum("connects_to_area_id", -1));
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
				PrintToServer("Section is currently: %s", section);
				
				decl String:spot_id[16] = "spot_0";
				for (int spot = 0; kv.JumpToKey(spot_id, false); Format(spot_id, sizeof(spot_id), "spot_%d", ++spot)) {
					int hiding_spot_id = kv.GetNum("id", -1);
					int flags = kv.GetNum("flags", -1);
					
					kv.JumpToKey("pos", false);
					float hiding_spot_origin[3];
					kv.GetVector("origin", hiding_spot_origin);
					kv.GoBack();
					
					PrintToServer("area id %d has hiding spot id %d (pos: %f %f %f), flags: %d",
						area_id, hiding_spot_id,
						hiding_spot_origin[0], hiding_spot_origin[1], hiding_spot_origin[2],
						flags);
					kv.GoBack();
				}
				kv.GoBack();
			}
			kv.GoBack();
		}
	}
	
	return false;
}
#endif // _NT_BOTS_NAV_PARSER_INC_