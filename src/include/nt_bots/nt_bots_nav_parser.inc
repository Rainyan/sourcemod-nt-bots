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
// Each header value is 4 bytes.
#define BYTES_PER_HEADER_VALUE 4
    int metaInfo[3];
    if (ReadFile(navFile, metaInfo, sizeof(metaInfo), BYTES_PER_HEADER_VALUE) != sizeof(metaInfo)) {
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
            PrintToServer("All OK.\nMagic: %x\nVersion: %i\nSize: %i",
                metaInfo[META_NAV_IDX_MAGIC],
                metaInfo[META_NAV_IDX_VERSION],
                metaInfo[META_NAV_IDX_BSPSIZE]);
        }
    }
    
    // Bail out early if we have an invalid nav file.
    if (!success) {
        return false;
    }
        
    int numPlaces = 0;
    if (ReadFileCell(navFile, numPlaces, BYTES_PER_HEADER_VALUE) != 1) {
        PrintToServer("Failed to parse nav num places");
        return false;
    } else if (numPlaces < 1) {
        PrintToServer("No places in nav");
        return false;
    }
    
    // Iterate places
    
    return success;
}

#endif // _NT_BOTS_NAV_PARSER_INC_