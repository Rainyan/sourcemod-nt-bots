#if !defined(_NT_BOTS_NAV_PARSER_INC_)
#define _NT_BOTS_NAV_PARSER_INC_

bool ParseNavOfMap(const char[] mapName)
{
    decl String:navPath[PLATFORM_MAX_PATH];
    if (Format(navPath, sizeof(navPath), "maps/%s.nav", mapName) < 1) {
        ThrowError("Format failed");
    } else if (!FileExists(navPath)) {
        PrintToServer("File does not exist: %s", navPath);
        return false;
    }
    
    File navFile = OpenFile(navPath, "r");
    if (navFile == null) {
        PrintToServer("Failed to open file: %s", navPath);
        return false;
    }
    
    bool success = ParseNavFile(navFile, navPath, mapName);
    delete navFile;
    return success;
}

// Parse based on valid file handle.
// Caller is responsible for the File memory and for ensuring
// the provided navPath and mapName are valid.
bool ParseNavFile(File navFile, const char[] navPath, const char[] mapName)
{
    // CNavMesh::Save
    
    bool success = false;
// Magic header. Defined in nav.h.
#define NAV_MAGIC_NUMBER 0xFEEDFACE
// This is the version NT engine uses.
// Versions are documented in CNavMesh::Save.
#define NAV_NEO_VERSION 9
// Indices for grabbing the nav header.
// Note that we're skipping sub-version header,
// because it was implemented in major version 10 (and we're on 9).
#define META_NAV_IDX_MAGIC 0
#define META_NAV_IDX_VERSION 1
#define META_NAV_IDX_BSPSIZE 2
    int metaInfo[3];
    if (ReadFile(navFile, metaInfo, sizeof(metaInfo), 4) != sizeof(metaInfo)) {
        PrintToServer("Failed to read meta info from file: %s", navPath);
    } else if (metaInfo[META_NAV_IDX_MAGIC] != NAV_MAGIC_NUMBER) {
        PrintToServer("Magic number mismatch: %s (%x vs %x)",
            navPath, metaInfo[META_NAV_IDX_MAGIC], NAV_MAGIC_NUMBER);
    } else if (metaInfo[META_NAV_IDX_VERSION] != NAV_NEO_VERSION) {
        PrintToServer("Nav file version mismatch: %s (was %i, expected %i)", 
            navPath, metaInfo[META_NAV_IDX_VERSION], NAV_NEO_VERSION);
    } else {
        decl String:bspPath[PLATFORM_MAX_PATH];
        if (Format(bspPath, sizeof(bspPath), "maps/%s.bsp", mapName) < 1) {
            ThrowError("Format failed");
        } else if (!FileExists(bspPath)) {
            PrintToServer("File does not exist: %s", bspPath);
        } else if (metaInfo[META_NAV_IDX_BSPSIZE] != FileSize(bspPath)) {
            PrintToServer("BSP file size mismatch from nav meta: %s (%i vs %i)",
                bspPath, metaInfo[META_NAV_IDX_BSPSIZE], FileSize(bspPath));
        } else {
            success = true;
            PrintToServer("Magic: %x\nNav version: %i\nBSP size: %i\nHeader OK.",
                metaInfo[META_NAV_IDX_MAGIC],
                metaInfo[META_NAV_IDX_VERSION],
                metaInfo[META_NAV_IDX_BSPSIZE]);
        }
    }
    
    // Bail out early if we have an invalid nav file.
    if (!success) {
        return false;
    }
        
    // PlaceDirectory::Save
    int numPlaces;
    if (!navFile.ReadUint16(numPlaces)) {
        PrintToServer("Failed to read numPlaces");
        return false;
    } else if (numPlaces < 0) {
        PrintToServer("Invalid places in nav; got: %u (hex %x)",
            numPlaces, numPlaces);
        return false;
    } else if (numPlaces > 1000) { // TODO: set upper bound properly
        PrintToServer("Unreasonable numPlaces: %u (hex %x), \
probably a parsing error.", numPlaces, numPlaces);
        return false;
    } else {
        PrintToServer("Num places: %u", numPlaces);
    }
    
    // Iterate places
    for (int placeIt = 0; placeIt < numPlaces; ++placeIt) {
        int placeNameLen;
        if (!navFile.ReadUint16(placeNameLen)) {
            PrintToServer("Failed to read placeNameLen");
            return false;
        }
        decl String:placeName[placeNameLen];
        if (ReadFileString(navFile, placeName,
            placeNameLen, placeNameLen) != placeNameLen)
        {
            PrintToServer("Failed to read placeName");
            return false;
        }
        placeName[placeNameLen] = '\0';
        PrintToServer("Place name: %s", placeName);
        
        // We don't do m_hasUnnamedAreas (version 12+ feature).
        
        int navAreaCount;
        if (ReadFileCell(navFile, navAreaCount, 4) != 1) {
            PrintToServer("Failed to read navAreaCount");
            return false;
        }
        
        for (int navAreaIt = 0; navAreaIt < navAreaCount; ++navAreaIt) {
            int id;
            if (ReadFileCell(navFile, id, 4) != 1) {
                PrintToServer("Failed to read nav area id");
                return false;
            }
            PrintToServer("Nav area id: %i", id);
            
            int attributeFlags;
            if (ReadFileCell(navFile, attributeFlags, 2) != 1) {
                PrintToServer("Failed to read attributeFlags");
                return false;
            } 
            PrintToServer("attributeFlags: %i", attributeFlags);
            
            // Extent of area
            float minsmaxs[6];
            if (ReadFileFloat(navFile, minsmaxs,
                sizeof(minsmaxs)) != sizeof(minsmaxs))
            {
                PrintToServer("Failed to read minsmaxs");
                return false;
            }
            PrintToServer("mins: %f %f %f\nmaxs: %f %f %f",
                minsmaxs[0], minsmaxs[1], minsmaxs[2],
                minsmaxs[3], minsmaxs[4], minsmaxs[5]);
            
            // Heights of implicit corners
#define Z_HEIGHT_NE 0
#define Z_HEIGHT_SW 1
            float zHeights[2];
            if (ReadFileFloat(navFile, zHeights,
                sizeof(zHeights)) != sizeof(zHeights))
            {
                PrintToServer("Failed to read zHeights");
                return false;
            }
            PrintToServer("zHeights_NE: %f\nzHeights_SW: %f",
                zHeights[Z_HEIGHT_NE], zHeights[Z_HEIGHT_SW]);
            
            // Connections to adjacent areas
#define NORTH           0
#define EAST            1
#define SOUTH           2
#define WEST            3
#define NUM_DIRECTIONS  4
            new const String:dirNames[NUM_DIRECTIONS][] = {
                "north", "east", "south", "west"
            };

            for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
                int numConnections;
                if (ReadFileCell(navFile, numConnections, 4) != 1) {
                    PrintToServer("Failed to read numConnections");
                    return false;
                } else if (numConnections > 100) {
                    PrintToServer("Unreasonable numConnections (%i), \
    probably a parsing error.", numConnections);
                    return false;
                }
                
                PrintToServer("Num connections: %i", numConnections);
                int[] connections = new int[numConnections];
                for (int connIt = 0; connIt < numConnections; ++connIt) {
                    PrintToServer("Iterating connection %i", connIt);
                    if (ReadFileCell(navFile, connections[connIt], 4) != 1) {
                        PrintToServer("Failed to read connection id");
                        return false;
                    }
                    PrintToServer("In place %i, area %i has a %sern connection to area %i",
                        placeIt,
                        navAreaIt,
                        dirNames[dir],
                        connections[connIt]);
                }
            }
        }
    }
    
    int navAreaCount;
    if (ReadFileCell(navFile, navAreaCount, 4) != 1) {
        PrintToServer("Failed to read navAreaCount");
        return false;
    }
    
    for (int navAreaIt = 0; navAreaIt < navAreaCount; ++navAreaIt) {
        int id;
        if (ReadFileCell(navFile, id, 4) != 1) {
            PrintToServer("Failed to read nav area id");
            return false;
        }
        PrintToServer("Nav area id: %i", id);
        
        int attributeFlags;
        if (ReadFileCell(navFile, attributeFlags, 2) != 1) {
            PrintToServer("Failed to read attributeFlags");
            return false;
        } 
        PrintToServer("attributeFlags: %i", attributeFlags);
        
        // Extent of area
        float minsmaxs[6];
        if (ReadFileFloat(navFile, minsmaxs,
            sizeof(minsmaxs)) != sizeof(minsmaxs))
        {
            PrintToServer("Failed to read minsmaxs");
            return false;
        }
        PrintToServer("mins: %f %f %f\nmaxs: %f %f %f",
            minsmaxs[0], minsmaxs[1], minsmaxs[2],
            minsmaxs[3], minsmaxs[4], minsmaxs[5]);
        
        // Heights of implicit corners
#define Z_HEIGHT_NE 0
#define Z_HEIGHT_SW 1
        float zHeights[2];
        if (ReadFileFloat(navFile, zHeights,
            sizeof(zHeights)) != sizeof(zHeights))
        {
            PrintToServer("Failed to read zHeights");
            return false;
        }
        PrintToServer("zHeights_NE: %f\nzHeights_SW: %f",
            zHeights[Z_HEIGHT_NE], zHeights[Z_HEIGHT_SW]);
        
        // Connections to adjacent areas
#define NORTH           0
#define EAST            1
#define SOUTH           2
#define WEST            3
#define NUM_DIRECTIONS  4
        new const String:dirNames[NUM_DIRECTIONS][] = {
            "north", "east", "south", "west"
        };

        for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
            int numConnections;
            if (ReadFileCell(navFile, numConnections, 4) != 1) {
                PrintToServer("Failed to read numConnections");
                return false;
            } else if (numConnections > 100) {
                PrintToServer("Unreasonable numConnections (%i), \
probably a parsing error.", numConnections);
                return false;
            }
            
            PrintToServer("Num connections: %i", numConnections);
            int[] connections = new int[numConnections];
            for (int connIt = 0; connIt < numConnections; ++connIt) {
                PrintToServer("Iterating connection %i", connIt);
                if (ReadFileCell(navFile, connections[connIt], 4) != 1) {
                    PrintToServer("Failed to read connection id");
                    return false;
                }
                PrintToServer("Area %i has a %sern connection to area %i",
                    navAreaIt,
                    dirNames[dir],
                    connections[connIt]);
            }
        }
    }
    
    int ladderCount;
    if (ReadFileCell(navFile, ladderCount, 4) != 1) {
        PrintToServer("Failed to read ladderCount");
        return false;
    }
    
    for (int ladderIt = 0; ladderIt < ladderCount; ++ladderIt) {
        int id;
        if (ReadFileCell(navFile, id, 4) != 1) {
            PrintToServer("Failed to read ladder id");
            return false;
        }
        PrintToServer("Ladder id: %i", id);
        
        float ladderWidth[1];
        if (ReadFileFloat(navFile, ladderWidth, 1) != 1) {
            PrintToServer("Failed to read ladderWidth");
            return false;
        }
        
        float ladderTopEndPoint[1];
        if (ReadFileFloat(navFile, ladderTopEndPoint, 1) != 1) {
            PrintToServer("Failed to read ladderTopEndPoint");
            return false;
        }
        
        float ladderBottomEndPoint[1];
        if (ReadFileFloat(navFile, ladderBottomEndPoint, 1) != 1) {
            PrintToServer("Failed to read ladderBottomEndPoint");
            return false;
        }
        
        float ladderLength[1];
        if (ReadFileFloat(navFile, ladderLength, 1) != 1) {
            PrintToServer("Failed to read ladderLength");
            return false;
        }
        
        int navDir;
        if (ReadFileCell(navFile, navDir, 4) != 1) {
            PrintToServer("Failed to read ladder navDir");
            return false;
        }
        
#define CONNAREA_TOP_FORWARD    0
#define CONNAREA_TOP_LEFT       1
#define CONNAREA_TOP_RIGHT      2
#define CONNAREA_TOP_BEHIND     3
#define CONNAREA_BOTTOM         4
#define NUM_CONNAREAS           5
        int connectingAreaIds[NUM_CONNAREAS];
        for (int connAIt = 0; connAIt < sizeof(connectingAreaIds); ++connAIt) {
            if (ReadFileCell(navFile, connectingAreaIds[connAIt], 4) != 1) {
                PrintToServer("Failed to read ladder connectingAreaId at %i",
                    connAIt);
                return false;
            }
        }
    }
    
    PrintToServer("Reached end");
    
    return true;
}

// Reads binary data from a file, and interprets it as contiguous float array.
// Returns number of elements read, or -1 on error.
int ReadFileFloat(File file, float[] items, int num_items)
{
#define SIZEOF_FLOAT_BYTES 4
#define NUM_BYTES num_items * SIZEOF_FLOAT_BYTES
    decl String:buff[NUM_BYTES + 1];
    buff[NUM_BYTES] = '\0';
    if (ReadFileString(file, buff, NUM_BYTES + 1, NUM_BYTES) != NUM_BYTES) {
        return -1;
    }
    int i;
    for (i = 0; i < num_items; ++i) {
        items[i] = view_as<float>(buff[i * SIZEOF_FLOAT_BYTES]);
    }
    return i;
}

#endif // _NT_BOTS_NAV_PARSER_INC_