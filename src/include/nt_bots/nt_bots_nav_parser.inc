#if !defined(_NT_BOTS_NAV_PARSER_INC_)
#define _NT_BOTS_NAV_PARSER_INC_

bool ParseNavOfMap(const char[] mapName)
{
	decl String:navPath[PLATFORM_MAX_PATH];
	if (Format(navPath, sizeof(navPath), "maps/%s.kv", mapName) < 1) {
		ThrowError("Format failed");
	} else if (!FileExists(navPath)) {
		PrintToServer("File does not exist: %s", navPath);
		return false;
	}
	
	KeyValues kv = new KeyValues("nav");
	if (!kv.ImportFromFile(navPath)) {
		SetFailState("Failed to import map \"%s\" nav file: \"%s\"", mapName, navPath);
	}
	
	bool success = ParseNavFile(kv, navPath);
	
	delete kv;
	
	return success;
}

// Parse based on valid file handle.
// Caller is responsible for the KeyValues handle memory management.
bool ParseNavFile(const KeyValues& kv, const char[] navPath)
{
	if (!kv.GotoFirstSubKey()) {
		return false;
	}
	
	if (!kv.JumpToKey("meta")) {
		return false;
	} else {
		if (kv.GetNum("nav_parse_successful", 0) != 1) {
			return false;
		}
	}
	
	if (!kv.JumpToKey("header")) {
		return false;
	} else {
		decl String:magic[9];
		if (kv.GetString("magic", magic, sizeof(magic), "\0")) {
			return false;
		} else if (!StrEqual(magic, "FEEDFACE", false)) {
			return false;
		}
		
		if (kv.GetNum("version", 0) != 9) {
			return false;
		}
		
		int nav_reported_bsp_size = kv.GetNum("bsp_size", 0);
		if (nav_reported_bsp_size != FileSize(navPath)) {
			return false;
		}
	}
	
	if (!kv.JumpToKey("navigation_areas")) {
		return false;
	} else {
		int area = 0;
		decl String:areaid[16];
		Format(areaid, sizeof(areaid), "area_%d", area);
		for (++area; kv.JumpToKey(areaid); Format(areaid, sizeof(areaid), "area_%d", area)) {
			int area_id = kv.GetNum("id", -1);
			if (area_id == -1) {
				return false;
			}
			
			// TODO: area stuff
		}
	}
	
	return true;
}
#endif // _NT_BOTS_NAV_PARSER_INC_