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
            
            // HidingSpot objects
            int hidingSpotCount;
            if (!navFile.ReadUint8(hidingSpotCount)) {
                PrintToServer("Failed to read hidingSpotCount");
                return false;
            }
            PrintToServer("hidingSpotCount: %i", hidingSpotCount);
            
            for (int hsIt = 0; hsIt < hidingSpotCount; ++hsIt) {
                int hidingSpotId;
                if (!navFile.ReadInt32(hidingSpotId)) {
                    PrintToServer("Failed to read hidingSpotCount");
                    return false;
                }
                
                float hsPos[3];
                if (ReadFileFloat(navFile, hsPos,
                    sizeof(hsPos)) != sizeof(hsPos))
                {
                    PrintToServer("Failed to read hsPos");
                    return false;
                }
                
                int hsFlags;
                if (!navFile.ReadUint8(hsFlags)) {
                    PrintToServer("Failed to read hsFlags");
                    return false;
                }
                
                PrintToServer("Hiding spot %i as pos: %f %f %f (flags %i)",
                    hsIt, hsPos[0], hsPos[1], hsPos[2], hsFlags);
            }
            
            // Approach area info
            int numApproachAreas;
            if (!navFile.ReadUint8(numApproachAreas)) {
                PrintToServer("Failed to read numApproachAreas");
                return false;
            }
            
            for (int apprIt = 0; apprIt < numApproachAreas; ++apprIt) {
                int approachHereAreaId;
                if (!navFile.ReadInt32(approachHereAreaId)) {
                    PrintToServer("Failed to read approachHereAreaId");
                    return false;
                }
                
                int approachPrevAreaId;
                if (!navFile.ReadInt32(approachPrevAreaId)) {
                    PrintToServer("Failed to read approachPrevAreaId");
                    return false;
                }
                
                int approachPrevToHereHowType;
                if (!navFile.ReadUint8(approachPrevToHereHowType)) {
                    PrintToServer("Failed to read approachPrevToHereHowType");
                    return false;
                }
                
                PrintToServer("%i Approach here area: %i, prev: %i, type: %i",
                    apprIt,
                    approachHereAreaId,
                    approachPrevAreaId,
                    approachPrevToHereHowType);
            }
            
            // Encounter spots for this area
            int encounterSpotCount;
            if (!navFile.ReadInt32(encounterSpotCount)) {
                PrintToServer("Failed to read encounterSpotCount");
                return false;
            }
            
            for (int encIt = 0; encIt < encounterSpotCount; ++encIt) {
                int encFromAreaId;
                if (!navFile.ReadInt32(encFromAreaId)) {
                    PrintToServer("Failed to read encFromAreaId");
                    return false;
                }
                
                int fromDir;
                if (!navFile.ReadUint8(fromDir)) {
                    PrintToServer("Failed to read fromDir");
                    return false;
                }
                
                int encToAreaId;
                if (!navFile.ReadInt32(encToAreaId)) {
                    PrintToServer("Failed to read encToAreaId");
                    return false;
                }
                
                int toDir;
                if (!navFile.ReadUint8(toDir)) {
                    PrintToServer("Failed to read toDir");
                    return false;
                }
                
                // List of spots along this path
                int spotsCount;
                if (!navFile.ReadUint8(spotsCount)) {
                    PrintToServer("Failed to read spotsCount");
                    return false;
                }
                
                for (int spotIt = 0; spotIt < spotsCount; ++spotIt) {
                    int spotId;
                    if (!navFile.ReadInt32(spotId)) {
                        PrintToServer("Failed to read spotId");
                        return false;
                    } else if (spotId == 0) {
                        // If spotId == 0, order->spot == NULL. This happens 
                        // if we've loaded a nav mesh that has been edited 
                        // but not re-analyzed.
                        PrintToServer("spotId at it %i == 0; spot is NULL",
                            spotIt, spotId);
                    } else {
                        // Parametric distance along ray where this spot
                        // first has LOS to our path. This is C cast from
                        // float -> (uchar)(255*float).
                        int t;
                        if (!navFile.ReadUint8(t)) {
                            PrintToServer("Failed to read t");
                            return false;
                        }
                        PrintToServer("spot t: %i", t);
                    }
                }
            }
            
            int placeDictEntry;
            if (!navFile.ReadUint16(placeDictEntry)) {
                PrintToServer("Failed to read placeDictEntry");
                return false;
            }
            PrintToServer("placeDictEntry: %i", placeDictEntry);
            
#define LADDER_DIR_UP   0
#define LADDER_DIR_DOWN 1
#define LADDER_NUM_DIRS 2
            for (int ladDir = 0; ladDir < LADDER_NUM_DIRS; ++ladDir) {
                // Number of encounter paths for this area
                int numLadderPaths;
                if (!navFile.ReadInt32(numLadderPaths)) {
                    PrintToServer("Failed to read numLadderPaths");
                    return false;
                }
                
                for (int ladPath = 0; ladPath < numLadderPaths; ++ladPath) {
                    int ladderId;
                    if (!navFile.ReadInt32(ladderId)) {
                        PrintToServer("Failed to read ladderId");
                        return false;
                    }
                }
            }

#define NAV_TEAM_FIRST 0
#define NAV_TEAM_SECOND 1
#define MAX_NAV_TEAMS 2
            // No spot in the map should take longer than this to reach
            float earliestOccupyTimes[MAX_NAV_TEAMS];
            if (ReadFileFloat(navFile, earliestOccupyTimes,
                sizeof(earliestOccupyTimes)) != sizeof(earliestOccupyTimes))
            {
                PrintToServer("Failed to read earliestOccupyTimes[]");
                return false;
            }
            PrintToServer("earliestOccupyTimes: %f and %f",
                earliestOccupyTimes[NAV_TEAM_FIRST],
                earliestOccupyTimes[NAV_TEAM_SECOND]);
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
        
        PrintToServer("About to read minsmaxs");
        
        if (ReadFileFloat(navFile, minsmaxs,
            sizeof(minsmaxs)) != sizeof(minsmaxs))
        {
            PrintToServer("Failed to read minsmaxs");
            return false;
        }
        PrintToServer("mins: %f %f %f\nmaxs: %f %f %f",
            minsmaxs[0], minsmaxs[1], minsmaxs[2],
            minsmaxs[3], minsmaxs[4], minsmaxs[5]);
        
        PrintToServer("Done!!");
        return false;
        
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
        
        // HidingSpot objects
        int hidingSpotCount;
        if (!navFile.ReadUint8(hidingSpotCount)) {
            PrintToServer("Failed to read hidingSpotCount");
            return false;
        }
        PrintToServer("hidingSpotCount: %i", hidingSpotCount);
        
        for (int hsIt = 0; hsIt < hidingSpotCount; ++hsIt) {
            int hidingSpotId;
            if (!navFile.ReadInt32(hidingSpotId)) {
                PrintToServer("Failed to read hidingSpotCount");
                return false;
            }
            
            float hsPos[3];
            if (ReadFileFloat(navFile, hsPos,
                sizeof(hsPos)) != sizeof(hsPos))
            {
                PrintToServer("Failed to read hsPos");
                return false;
            }
            
            int hsFlags;
            if (!navFile.ReadUint8(hsFlags)) {
                PrintToServer("Failed to read hsFlags");
                return false;
            }
            
            PrintToServer("Hiding spot %i as pos: %f %f %f (flags %i)",
                hsIt, hsPos[0], hsPos[1], hsPos[2], hsFlags);
        }
        
        // Approach area info
        int numApproachAreas;
        if (!navFile.ReadUint8(numApproachAreas)) {
            PrintToServer("Failed to read numApproachAreas");
            return false;
        }
        
        for (int apprIt = 0; apprIt < numApproachAreas; ++apprIt) {
            int approachHereAreaId;
            if (!navFile.ReadInt32(approachHereAreaId)) {
                PrintToServer("Failed to read approachHereAreaId");
                return false;
            }
            
            int approachPrevAreaId;
            if (!navFile.ReadInt32(approachPrevAreaId)) {
                PrintToServer("Failed to read approachPrevAreaId");
                return false;
            }
            
            int approachPrevToHereHowType;
            if (!navFile.ReadUint8(approachPrevToHereHowType)) {
                PrintToServer("Failed to read approachPrevToHereHowType");
                return false;
            }
            
            PrintToServer("%i Approach here area: %i, prev: %i, type: %i",
                apprIt,
                approachHereAreaId,
                approachPrevAreaId,
                approachPrevToHereHowType);
        }
        
        // Encounter spots for this area
        int encounterSpotCount;
        if (!navFile.ReadInt32(encounterSpotCount)) {
            PrintToServer("Failed to read encounterSpotCount");
            return false;
        }
        
        for (int encIt = 0; encIt < encounterSpotCount; ++encIt) {
            int encFromAreaId;
            if (!navFile.ReadInt32(encFromAreaId)) {
                PrintToServer("Failed to read encFromAreaId");
                return false;
            }
            
            int fromDir;
            if (!navFile.ReadUint8(fromDir)) {
                PrintToServer("Failed to read fromDir");
                return false;
            }
            
            int encToAreaId;
            if (!navFile.ReadInt32(encToAreaId)) {
                PrintToServer("Failed to read encToAreaId");
                return false;
            }
            
            int toDir;
            if (!navFile.ReadUint8(toDir)) {
                PrintToServer("Failed to read toDir");
                return false;
            }
            
            // List of spots along this path
            int spotsCount;
            if (!navFile.ReadUint8(spotsCount)) {
                PrintToServer("Failed to read spotsCount");
                return false;
            }
            
            for (int spotIt = 0; spotIt < spotsCount; ++spotIt) {
                int spotId;
                if (!navFile.ReadInt32(spotId)) {
                    PrintToServer("Failed to read spotId");
                    return false;
                } else if (spotId == 0) {
                    // If spotId == 0, order->spot == NULL. This happens 
                    // if we've loaded a nav mesh that has been edited 
                    // but not re-analyzed.
                    PrintToServer("spotId at it %i == 0; spot is NULL",
                        spotIt, spotId);
                } else {
                    // Parametric distance along ray where this spot
                    // first has LOS to our path. This is C cast from
                    // float -> (uchar)(255*float).
                    int t;
                    if (!navFile.ReadUint8(t)) {
                        PrintToServer("Failed to read t");
                        return false;
                    }
                    PrintToServer("spot t: %i", t);
                }
            }
        }
        
        int placeDictEntry;
        if (!navFile.ReadUint16(placeDictEntry)) {
            PrintToServer("Failed to read placeDictEntry");
            return false;
        }
        PrintToServer("placeDictEntry: %i", placeDictEntry);
        
#define LADDER_DIR_UP   0
#define LADDER_DIR_DOWN 1
#define LADDER_NUM_DIRS 2
        for (int ladDir = 0; ladDir < LADDER_NUM_DIRS; ++ladDir) {
            // Number of encounter paths for this area
            int numLadderPaths;
            if (!navFile.ReadInt32(numLadderPaths)) {
                PrintToServer("Failed to read numLadderPaths");
                return false;
            }
            
            for (int ladPath = 0; ladPath < numLadderPaths; ++ladPath) {
                int ladderId;
                if (!navFile.ReadInt32(ladderId)) {
                    PrintToServer("Failed to read ladderId");
                    return false;
                }
            }
        }

#define NAV_TEAM_FIRST 0
#define NAV_TEAM_SECOND 1
#define MAX_NAV_TEAMS 2
        // No spot in the map should take longer than this to reach
        float earliestOccupyTimes[MAX_NAV_TEAMS];
        if (ReadFileFloat(navFile, earliestOccupyTimes,
            sizeof(earliestOccupyTimes)) != sizeof(earliestOccupyTimes))
        {
            PrintToServer("Failed to read earliestOccupyTimes[]");
            return false;
        }
        PrintToServer("earliestOccupyTimes: %f and %f",
            earliestOccupyTimes[NAV_TEAM_FIRST],
            earliestOccupyTimes[NAV_TEAM_SECOND]);
    }
    
    ///////////////
#if(0)
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
#endif
    
    PrintToServer("Reached end");
    
    return true;
}

// Reads binary data from a file, and interprets it as contiguous float array.
// Returns number of elements read, or -1 on error.
int ReadFileFloat(File file, float[] items, int num_items)
{
    if (num_items < 1) {
        return 0;
    }
    
#define SIZEOF_FLOAT_BYTES 4
#define NUM_BYTES num_items * SIZEOF_FLOAT_BYTES
    decl String:byteBuffer[NUM_BYTES];
    if (ReadFileString(file, byteBuffer, NUM_BYTES, NUM_BYTES) != NUM_BYTES) {
        LogError("Read failed");
        return -1;
    }
    
    // Max characters for storing significant figures. Increase if you need more.
#define FLOAT_STR_REPRESENTATION_MAXLEN 31
    decl String:floatString[FLOAT_STR_REPRESENTATION_MAXLEN + 1];
    
    int i;
    for (i = 0; i < num_items; ++i) {
        // SourcePawn (1.10.0.6478) gets confused if we do a raw byte cast
        // on the array pointer, so routing the reinterpret via string format.
        if (Format(floatString, FLOAT_STR_REPRESENTATION_MAXLEN + 1,
            "%f", view_as<float>(byteBuffer[i * SIZEOF_FLOAT_BYTES])) < 1)
        {
            LogError("Float format failed");
            return -1;
        }
        items[i] = StringToFloat(floatString);
        PrintToServer("String res: %s as float: %f",
            floatString, items[i]);
    }
    return i;
}

#endif // _NT_BOTS_NAV_PARSER_INC_