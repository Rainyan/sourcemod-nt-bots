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

bool ReadMetaHeader(File navFile, const char[] navPath, const char[] mapName)
{
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

#define ITEM_SIZE_BYTES 4
    int metaInfo[3];
    int totalSeeked = ReadFile(navFile, metaInfo,
        sizeof(metaInfo), ITEM_SIZE_BYTES);
    
    if (totalSeeked != sizeof(metaInfo))
    {
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
            PrintToServer("| Magic: %x\n| Nav version: %i\n| BSP size: %i\n\
| Nav size: %i\n| Header OK: %s.",
                metaInfo[META_NAV_IDX_MAGIC],
                metaInfo[META_NAV_IDX_VERSION],
                metaInfo[META_NAV_IDX_BSPSIZE],
                FileSize(navPath),
                (success == true ? "yes" : "no"));
        }
    }
    
    int offsetBytes = totalSeeked * ITEM_SIZE_BYTES;
    PrintToServer("Offset items: %i, bytes: %i",
        totalSeeked, offsetBytes);
    navFile.Seek(offsetBytes, 0);
    
    return success;
}

int ReadPlaces(File navFile)
{
    // PlaceDirectory::Save
    int numPlaces;
    if (!navFile.ReadUint16(numPlaces)) {
        PrintToServer("Failed to read numPlaces");
        return -1;
    } else if (numPlaces < 0) {
        PrintToServer("Invalid places in nav; got: %u (hex %x)",
            numPlaces, numPlaces);
        return -1;
    } else if (numPlaces > 1000) { // TODO: set upper bound properly
        PrintToServer("Unreasonable numPlaces: %u (hex %x), \
probably a parsing error.", numPlaces, numPlaces);
        return -1;
    } else {
        PrintToServer("Num places: %u", numPlaces);
    }
    
    // Iterate places
    for (int placeIt = 0; placeIt < numPlaces; ++placeIt) {
        int placeNameLen;
        if (!navFile.ReadUint16(placeNameLen)) {
            PrintToServer("Failed to read placeNameLen");
            return -1;
        }
        decl String:placeName[placeNameLen];
        if (ReadFileString(navFile, placeName,
            placeNameLen, placeNameLen) != placeNameLen)
        {
            PrintToServer("Failed to read placeName");
            return -1;
        }
        placeName[placeNameLen] = '\0';
        PrintToServer("Place name: %s", placeName);
        
        // We don't do m_hasUnnamedAreas (version 12+ feature).
    }
    return numPlaces;
}

bool ReadAreas(File navFile)
{
    int navAreaCount[1];
    if (ReadFileUint32(navFile, navAreaCount, 1) != 1) {
        PrintToServer("Failed to read navAreaCount");
        return false;
    }
    PrintToServer("Got %i nav areas", navAreaCount[0]);
    
    for (int navAreaIt = 0; navAreaIt < navAreaCount[0]; ++navAreaIt) {
        int id[1];
        if (ReadFileUint32(navFile, id, 1) != 1) {
            PrintToServer("Failed to read nav id");
            return false;
        }
        PrintToServer("Nav area id: %i", id[0]);
        
        int attributeFlags;
        if (!navFile.ReadUint16(attributeFlags)) {
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
            int numConnections[1];
            if (ReadFileUint32(navFile, numConnections, 1) != 1) {
                PrintToServer("Failed to read numConnections");
                return false;
            } else if (numConnections[0] > 100) {
                PrintToServer("Unreasonable numConnections (%i), \
probably a parsing error.", numConnections[0]);
                return false;
            } else if (numConnections[0] == 0) {
                PrintToServer("No connections for %s", dirNames[dir]);
                continue;
            }
            
            PrintToServer("Num connections on %s: %i",
                dirNames[dir], numConnections[0]);
            
            int[] connections = new int[numConnections[0]];
            
            int readConnAreas = ReadFile(navFile, connections,
                numConnections[0], 4);
            if (readConnAreas == -1) {
                PrintToServer("readConnAreas read error");
                return false;
            } else if (readConnAreas != numConnections[0]) {
                PrintToServer("readConnAreas %i but expected %i",
                    readConnAreas, numConnections[0]);
            }
            
            for (int connIt = 0; connIt < readConnAreas; ++connIt) {
                PrintToServer("Area %i has a %sern connection to area %i",
                    id,
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
    return true;
}

bool ReadLadders(File navFile)
{
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
    return true;
}

// Parse based on valid file handle.
// Caller is responsible for the File memory and for ensuring
// the provided navPath and mapName are valid.
bool ParseNavFile(File navFile, const char[] navPath, const char[] mapName)
{
    // CNavMesh::Save
    
    int navSize = FileSize(navPath);
    PrintToServer("Nav size: %i", navSize);
    char[] buffer = new char[navSize];
    int written = navFile.ReadString(buffer, navSize, navSize);
    PrintToServer("Dumping %i written nav data:", written);
    //for (int i = 0; i < navSize; ++i) {
    //    PrintToServer("i %i: hex: %x binary: %b", i, buffer[i], buffer[i]);
    //}
    
    int offset = 0;
    
    PrintToServer("Magic: 0x%x", buffer[offset]);
    offset += 4;
    PrintToServer("Version: %i", buffer[offset]);
    offset += 4;
    PrintToServer("BspSize: %i", buffer[offset]);
    offset += 4;
    
    decl String:bitBuff[4];
    Format(bitBuff, sizeof(bitBuff), "%c%c%c%c",
        '\0', '\0', buffer[offset], buffer[offset + 1]);
    PrintToServer("hex: 0x%x bin: %b", bitBuff, bitBuff);
    int numPlaces = view_as<int>(bitBuff[0]);
    PrintToServer("Num places: %i (%x -- %b)",
        numPlaces, numPlaces, numPlaces);
    offset += 2;
    
    for (int i = 0; i < numPlaces; ++i) {
        Format(bitBuff, sizeof(bitBuff), "%c%c%c%c",
            '\0', '\0', buffer[offset], buffer[offset + 1]);
        int nameLen = bitBuff[0];
        PrintToServer("Name len: %i (%x -- %b)",
            nameLen, nameLen, nameLen);
        offset += 2;
        
        PrintToServer("Name: %s", buffer[offset]);
        offset += nameLen;
    }
    
    int navAreaCount = view_as<int>(buffer[offset]);
    PrintToServer("Nav area count: %i (from %i)",
        navAreaCount, buffer[offset]);
    offset += 4;
    
    for (int i = 0; i < navAreaCount; ++i) {
        int id = view_as<int>(buffer[offset]);
        offset += 4;
        PrintToServer("ID: %i", id);
        
        Format(bitBuff, sizeof(bitBuff), "%c%c%c%c",
            '\0', '\0', buffer[offset], buffer[offset + 1]);
        PrintToServer("hex: 0x%x bin: %b", bitBuff, bitBuff);
        int attribFlags = view_as<int>(bitBuff[0]);
        PrintToServer("attribFlags: %i (%x -- %b)",
            attribFlags, attribFlags, attribFlags);
        offset += 2;
        
        for (int j = 0; j < 6; ++j) {
            PrintToServer("ExtentOfArea%i: %f", j, buffer[offset]);
            offset += 4;
        }
        
        for (int j = 0; j < 2; ++j) {
            PrintToServer("HeightOfImplicitCorners%i: %f",
                j, buffer[offset]);
            offset += 4;
        }
        
        new const String:dirNames[NUM_DIRECTIONS][] = {
            "north", "east", "south", "west"
        };
        
        for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
            int count = view_as<int>(buffer[offset]);
            PrintToServer("Area %i has %i %sern connections.",
                id, count, dirNames[dir]);
            offset += 4;
            for (int countIt = 0; countIt < count; ++countIt) {
                PrintToServer("Area %i has %sern connection to area: %i",
                    id, dirNames[dir], buffer[offset]);
                offset += 4;
            }
        }
        
        char hidingSpotCount = view_as<char>(buffer[offset]);
        PrintToServer("hidingSpotCount: %u (%x -- %b)",
            hidingSpotCount, hidingSpotCount, hidingSpotCount);
        offset += 1;
        
        for (int j = 0; j < hidingSpotCount; ++j) {
            int hsId = view_as<int>(buffer[offset]); offset += 4;
            for (int k = 0; k < 3; ++k) {
                PrintToServer("%i Hidingspot %i: %f",
                    hsId, k, buffer[offset]);
                offset += 4;
            }
            PrintToServer("%i hsFlags: %c", hsId, buffer[offset]);
            offset += 1;
        }
        
        char approachAreaCount = view_as<char>(buffer[offset]);
        PrintToServer("approachAreaCount: %u (%x -- %b)",
            approachAreaCount, approachAreaCount, approachAreaCount);
        offset += 1;
        
        for (int j = 0; j < approachAreaCount; ++j) {
            PrintToServer("%i approach here area: %i",
                j, buffer[offset]);
            offset += 4;
            PrintToServer("%i approach prev area: %i",
                j, buffer[offset]);
            offset += 4;
            PrintToServer("%i prev to here how: %c",
                j, buffer[offset]);
            offset += 1;
            PrintToServer("%i approach next area: %i",
                j, buffer[offset]);
            offset += 4;
            PrintToServer("%i here to next how: %c",
                j, buffer[offset]);
            offset += 1;
        }
        
        int encounterSpotCount = view_as<int>(buffer[offset]);
        PrintToServer("encounterSpotCount: %i (%x -- %b)",
            encounterSpotCount, encounterSpotCount, encounterSpotCount);
        offset += 4;
        
        for (int j = 0; j < encounterSpotCount; ++j) {
            PrintToServer("%i enc from area: %i",
                j, buffer[offset]);
            offset += 4;
            PrintToServer("%i enc from dir: %c",
                j, buffer[offset]);
            offset += 1;
            PrintToServer("%i enc to area: %i",
                j, buffer[offset]);
            offset += 4;
            PrintToServer("%i enc to dir: %c",
                j, buffer[offset]);
            offset += 1;
            
            char spotCount = view_as<char>(buffer[offset]);
            PrintToServer("spotCount: %c (%x -- %b)",
                spotCount, spotCount, spotCount);
            offset += 1;
            
            for (int k = 0; k < spotCount; ++k) {
                PrintToServer("%i enc spot %i id: %i",
                    j, k, buffer[offset]);
                offset += 4;
                PrintToServer("%i enc spot %i t: %c",
                    j, k, buffer[offset]);
                offset += 1;
            }
        }
        
        Format(bitBuff, sizeof(bitBuff), "%c%c%c%c",
            '\0', '\0', buffer[offset], buffer[offset + 1]);
        PrintToServer("hex: 0x%x bin: %b", bitBuff, bitBuff);
        int placeDictIdxType = view_as<int>(bitBuff[0]);
        PrintToServer("placeDictIdxType: %i (%x -- %b)",
            placeDictIdxType, placeDictIdxType, placeDictIdxType);
        offset += 2;
        
        for (int j = 0; j < LADDER_NUM_DIRS; ++j) {
            int ladConnCount = view_as<int>(buffer[offset]);
            offset += 4;
            for (int l = 0; l < ladConnCount; ++l) {
                PrintToServer("Area %i ladder %i conn %i id: %i",
                    id, j, l, buffer[offset]);
                offset += 4;
            }
        }
        
        for (int j = 0; j < MAX_NAV_TEAMS; ++j) {
            PrintToServer("Area %i team %i earliest occupy time: %f",
                id, j, buffer[offset]);
            offset += 4;
        }

        int numLadders = view_as<int>(buffer[offset]);
        offset += 4;
        
        for (int j = 0; j < numLadders; ++j) {
            PrintToServer("Ladder %i id: %i", j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i extent: %f", j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i top endpoint: %f %f %f",
                j,
                buffer[offset + (0 * 4)],
                buffer[offset + (1 * 4)],
                buffer[offset + (2 * 4)]);
            offset += (3 * 4);
            
            PrintToServer("Ladder it %i bottom endpoint: %f %f %f",
                j,
                buffer[offset + (0 * 4)],
                buffer[offset + (1 * 4)],
                buffer[offset + (2 * 4)]);
            offset += (3 * 4);
            
            PrintToServer("Ladder it %i length: %f", j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i navdirtype enum: %i",
                j, buffer[offset]);
            offset += 4; // TODO: enum == 4?
            
            PrintToServer("Ladder it %i top forward area: %i",
                j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i top left area: %i",
                j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i top right area: %i",
                j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i top behind area: %i",
                j, buffer[offset]);
            offset += 4;
            
            PrintToServer("Ladder it %i bottom area: %i",
                j, buffer[offset]);
            offset += 4;
        }
    }
    
    PrintToServer("Consumed %i of %i bytes.",
        offset, navSize);
    
    return (navSize > 0 && offset == navSize);
    
#if(0)
    
    // Bail out early if we have an invalid nav file.
    if (!ReadMetaHeader(navFile, navPath, mapName)) {
        return false;
    }
    
    PrintToServer("| Consumed %i / %i bytes",
        FilePosition(navFile), FileSize(navPath));
    
    // Places
    int numPlaces = ReadPlaces(navFile);
    if (numPlaces == -1) {
        return false;
    }
    
    PrintToServer("| Consumed %i / %i bytes",
        FilePosition(navFile), FileSize(navPath));
    
    // Navigation areas
    if (!ReadAreas(navFile)) {
        return false;
    }
    
    PrintToServer("| Consumed %i / %i bytes",
        FilePosition(navFile), FileSize(navPath));
    
    // Ladders
    if (!ReadLadders(navFile)) {
        return false;
    }
    
    PrintToServer("| Consumed %i / %i bytes",
        FilePosition(navFile), FileSize(navPath));
    
    PrintToServer("Reached end");
    
    return true;
#endif
}
#endif // _NT_BOTS_NAV_PARSER_INC_