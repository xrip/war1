#include <string.h>

#include "file.h"
#include "core/log.h"

// Safe little-endian integer reads — avoid pointer-cast aliasing/alignment UB.
static u32 wfile_readU32LE(const u8* p)
{
    u32 v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static u16 wfile_readU16LE(const u8* p)
{
    u16 v;
    memcpy(&v, p, sizeof(v));
    return v;
}

// DATA.WAR (5.4 MB retail, 7.4 MB unpacked) does not fit next to the
// game in 8 MB of PSRAM. Keep the offset table and the file open; entries
// are decompressed when their resource is first loaded.

static void wfile_decompress(const u8* ptr, u32 compressedLength, u8* data, u32 length)
{
#define BUFWIN_SIZE 4096
    u8 bufwin[BUFWIN_SIZE];
    memset(bufwin, 0, BUFWIN_SIZE);

    u32 b = 0;
    u32 bufwinPos = 0;

    while (b < compressedLength && bufwinPos < length)
    {
        u8 cmask = *ptr++;
        b++;

        for (s32 a = 0; a < 8 && bufwinPos < length; ++a)
        {
            if (cmask & 1) // uncompressed byte
            {
                u8 bufbyte = *ptr++;
                b++;

                bufwin[bufwinPos % BUFWIN_SIZE] = bufbyte;
                data[bufwinPos] = bufbyte;
                bufwinPos++;
            }
            else // compressed block
            {
                u16 offset = wfile_readU16LE(ptr);
                ptr += 2;
                u16 numbytes = offset / BUFWIN_SIZE;
                offset = offset % BUFWIN_SIZE;
                b += 2;

                for (s32 m = 0; m <= numbytes + 2 && bufwinPos < length; ++m)
                {
                    u8 bufbyte = bufwin[(offset + m) % BUFWIN_SIZE];

                    bufwin[bufwinPos % BUFWIN_SIZE] = bufbyte;
                    data[bufwinPos] = bufbyte;
                    bufwinPos++;
                }
            }

            cmask >>= 1;
        }
    }
#undef BUFWIN_SIZE
}

WarFile* wfile_loadWarFile(WarContext* context, StringView filePath, const bool skipDecompress[])
{
    NOT_USED(context);

    FILE* stream = fopen(wsv_data(filePath), "rb");
    if (!stream)
    {
        logError("Couldn't process the DATA.WAR file. The file doesn't exists at %s.", wsv_data(filePath));
        return NULL;
    }

    fseek(stream, 0, SEEK_END);
    long fileLength = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    u8 header[8];
    if (fileLength < 8 || fread(header, 1, 8, stream) != 8)
    {
        logError("Couldn't read the DATA.WAR header.");
        fclose(stream);
        return NULL;
    }

    WarFile *warFile = (WarFile*)wm_alloc(sizeof(WarFile));
    if (!warFile)
    {
        fclose(stream);
        return NULL;
    }

    warFile->archiveID       = wfile_readU32LE(header + 0);
    warFile->numberOfEntries = wfile_readU32LE(header + 4);
    warFile->stream = stream;
    warFile->fileLength = (u32)fileLength;

    switch (warFile->archiveID)
    {
        case 0x18:
        case 0x1A: warFile->type = WAR_FILE_TYPE_RETAIL; break;
        case 0x19: warFile->type = WAR_FILE_TYPE_DEMO; break;
        default:   warFile->type = WAR_FILE_TYPE_UNKNOWN; break;
    }

    if (warFile->type == WAR_FILE_TYPE_UNKNOWN || warFile->numberOfEntries > MAX_RESOURCES_COUNT ||
        fread(warFile->offsets, sizeof(u32), warFile->numberOfEntries, stream) != warFile->numberOfEntries)
    {
        logError("Couldn't process the DATA.WAR file: type %u, %u entries.", warFile->archiveID, warFile->numberOfEntries);
        wfile_freeWarFile(warFile);
        return NULL;
    }

    for (s32 i = 0; i < (s32)warFile->numberOfEntries; ++i)
    {
        WarRawResource* raw = &warFile->resources[i];
        raw->placeholder = true;
        raw->index = i;

        if (warFile->offsets[i] == 0xFFFFFFFF || warFile->offsets[i] == 0x00000000)
            continue;
        if (skipDecompress && skipDecompress[i])
            continue;

        u32 nextOffset = (u32)fileLength;
        for (s32 j = i + 1; j < (s32)warFile->numberOfEntries; j++)
        {
            if (warFile->offsets[j] != 0xFFFFFFFF && warFile->offsets[j] != 0x00000000)
            {
                nextOffset = warFile->offsets[j];
                break;
            }
        }

        if (warFile->offsets[i] >= (u32)fileLength || nextOffset < warFile->offsets[i] + 4 ||
            nextOffset > (u32)fileLength)
        {
            logError("Resource %d: offset %u out of bounds (fileLength %ld).", i, warFile->offsets[i], fileLength);
            continue;
        }

        u8 size4[4];
        fseek(stream, (long)warFile->offsets[i], SEEK_SET);
        if (fread(size4, 1, 4, stream) != 4)
            continue;

        u32 size = wfile_readU32LE(size4);
        raw->offset = warFile->offsets[i];
        raw->compressedLength = nextOffset - warFile->offsets[i];
        raw->length = size & 0x1FFFFFFF;
        raw->compressed = (size & 0xE0000000) != 0;
        raw->data = NULL;

        if (!raw->compressed && raw->length > raw->compressedLength - 4)
        {
            logError("Resource %d: uncompressed length %u exceeds payload %u.", i, raw->length, raw->compressedLength - 4);
            continue;
        }

        raw->placeholder = false;
    }

    return warFile;
}

bool wfile_readResource(WarFile* warFile, s32 index)
{
    WarRawResource* raw = &warFile->resources[index];
    if (raw->placeholder)
        return false;
    if (raw->data)
        return true;

    u32 payload = raw->compressedLength - 4;
    u8* data = (u8*)wm_alloc(raw->length ? raw->length : 1);
    u8* packed = raw->compressed ? (u8*)wm_alloc(payload) : data;
    if (!data || !packed)
    {
        logError("Couldn't allocate memory for resource %d (%u bytes).", index, raw->length);
        if (data) wm_free(data);
        return false;
    }

    FILE* stream = (FILE*)warFile->stream;
    fseek(stream, (long)raw->offset + 4, SEEK_SET);
    u32 want = raw->compressed ? payload : raw->length;
    bool ok = fread(packed, 1, want, stream) == want;

    if (ok && raw->compressed)
        wfile_decompress(packed, payload, data, raw->length);
    if (raw->compressed)
        wm_free(packed);

    if (!ok)
    {
        logError("Couldn't read resource %d from DATA.WAR.", index);
        wm_free(data);
        return false;
    }

    raw->data = data;
    return true;
}

void wfile_releaseResource(WarFile* warFile, s32 index)
{
    WarRawResource* raw = &warFile->resources[index];
    if (raw->data)
    {
        wm_free(raw->data);
        raw->data = NULL;
    }
}

void wfile_freeWarFile(WarFile* warFile)
{
    if (!warFile)
        return;

    for (s32 i = 0; i < (s32)warFile->numberOfEntries; ++i)
        wfile_releaseResource(warFile, i);

    if (warFile->stream)
        fclose((FILE*)warFile->stream);

    wm_free(warFile);
}

// -----------------------------------------------------------------------
// .w1m custom map loading
// -----------------------------------------------------------------------
#define WFILE_W1M_MAGIC   0x57314D41u
#define WFILE_W1M_VERSION 2u

// Read a .w1m stream into a pre-allocated WarResource and two u16 tile arrays.
// path_ and stream_ are the parameter names expected by the macros above.
// Returns true on success; closes stream_ and returns false on any error.
bool wfile_loadWarMapFile(StringView filePath, WarResource* levelInfoRes, WarResource* visualInfoRes, WarResource* passableInfoRes)
{
    assert(levelInfoRes);
    assert(visualInfoRes);
    assert(passableInfoRes);

// WFILE_READ_FIELD: read one field of (sz_) bytes from stream_ into (ptr_).
// On failure, closes the stream and returns false.
#define WFILE_READ_FIELD(ptr_, sz_) \
    if (SDL_ReadIO(stream, (ptr_), (sz_)) != (size_t)(sz_)) \
    { \
        logError("wfile_loadWarMapFile: unexpected EOF in '%.*s'", \
                 (int)filePath.length, filePath.data); \
        SDL_CloseIO(stream); \
        return false; \
    }

// WFILE_READ_ARRAY: read (cnt_) elements of (esz_) bytes each from stream_ into (ptr_).
#define WFILE_READ_ARRAY(ptr_, esz_, cnt_) \
    if ((cnt_) > 0 && SDL_ReadIO(stream, (ptr_), (size_t)(esz_) * (size_t)(cnt_)) != (size_t)(esz_) * (size_t)(cnt_)) \
    { \
        logError("wfile_loadWarMapFile: unexpected EOF reading array in '%.*s'", \
                 (int)filePath.length, filePath.data); \
        SDL_CloseIO(stream); \
        return false; \
    }

    SDL_IOStream* stream = SDL_IOFromFile(wsv_data(filePath), "rb");
    if (!stream)
    {
        logError("wfile_loadWarMapFile: cannot open '%.*s'",
                 (int)filePath.length, filePath.data);
        return false;
    }

    WarMapFileHeader hdr;
    WFILE_READ_FIELD(&hdr, sizeof(hdr));

    if (hdr.magic != WFILE_W1M_MAGIC)
    {
        logError("wfile_loadWarMapFile: bad magic 0x%08X in '%.*s' (expected 0x%08X)",
                 hdr.magic, (int)filePath.length, filePath.data, WFILE_W1M_MAGIC);
        SDL_CloseIO(stream);
        return false;
    }
    if (hdr.version != WFILE_W1M_VERSION)
    {
        logError("wfile_loadWarMapFile: unsupported version %u in '%.*s' (expected %u)",
                 hdr.version, (int)filePath.length, filePath.data, WFILE_W1M_VERSION);
        SDL_CloseIO(stream);
        return false;
    }

    u8 allowedHuman  = 0u;
    u8 allowedOrcs   = 0u;
    u8 customMapByte = 0u;
    u8 pad           = 0u;

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.allowId,        sizeof(u32));
    WFILE_READ_FIELD(&allowedHuman,                           sizeof(u8));
    WFILE_READ_FIELD(&allowedOrcs,                            sizeof(u8));
    WFILE_READ_FIELD(&customMapByte,                          sizeof(u8));
    WFILE_READ_FIELD(&pad,                                    sizeof(u8));
    levelInfoRes->levelInfo.allowedHumanUnits = (allowedHuman  != 0);
    levelInfoRes->levelInfo.allowedOrcsUnits  = (allowedOrcs   != 0);
    levelInfoRes->levelInfo.customMap         = (customMapByte != 0);

    WFILE_READ_FIELD(levelInfoRes->levelInfo.allowedFeatures, sizeof(levelInfoRes->levelInfo.allowedFeatures));
    WFILE_READ_FIELD(levelInfoRes->levelInfo.allowedUpgrades, sizeof(levelInfoRes->levelInfo.allowedUpgrades));
    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startX,         sizeof(u16));
    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startY,         sizeof(u16));
    WFILE_READ_FIELD(&levelInfoRes->levelInfo.tilesetType,    sizeof(u16));
    WFILE_READ_FIELD(&pad, sizeof(u8));
    WFILE_READ_FIELD(&pad, sizeof(u8));
    WFILE_READ_FIELD(levelInfoRes->levelInfo.lumber,          sizeof(levelInfoRes->levelInfo.lumber));
    WFILE_READ_FIELD(levelInfoRes->levelInfo.gold,            sizeof(levelInfoRes->levelInfo.gold));
    WFILE_READ_FIELD(levelInfoRes->levelInfo.races,           sizeof(levelInfoRes->levelInfo.races));
    WFILE_READ_FIELD(&pad, sizeof(u8));
    WFILE_READ_FIELD(&pad, sizeof(u8));
    WFILE_READ_FIELD(&pad, sizeof(u8));
    WFILE_READ_FIELD(levelInfoRes->levelInfo.objectives,      MAX_OBJECTIVES_LENGTH);
    levelInfoRes->levelInfo.objectives[MAX_OBJECTIVES_LENGTH - 1] = '\0';

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startEntitiesCount, sizeof(u32));
    if (levelInfoRes->levelInfo.startEntitiesCount > MAX_ENTITIES_COUNT)
    {
        logError("wfile_loadWarMapFile: startEntitiesCount %u too large in '%.*s'",
                 levelInfoRes->levelInfo.startEntitiesCount,
                 (int)filePath.length, filePath.data);
        SDL_CloseIO(stream);
        return false;
    }
    levelInfoRes->levelInfo.startEntities = (WarLevelUnit*)wm_alloc(MAX_ENTITIES_COUNT * sizeof(WarLevelUnit));
    if (!levelInfoRes->levelInfo.startEntities)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for startEntities in '%.*s'",
                 (int)filePath.length, filePath.data);
        SDL_CloseIO(stream);
        return false;
    }
    WFILE_READ_ARRAY(levelInfoRes->levelInfo.startEntities, sizeof(WarLevelUnit), levelInfoRes->levelInfo.startEntitiesCount);

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startRoadsCount, sizeof(u32));
    if (levelInfoRes->levelInfo.startRoadsCount > MAX_CONSTRUCTS_COUNT)
    {
        logError("wfile_loadWarMapFile: startRoadsCount %u too large in '%.*s'",
                 levelInfoRes->levelInfo.startRoadsCount,
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        SDL_CloseIO(stream);
        return false;
    }
    levelInfoRes->levelInfo.startRoads = (WarLevelConstruct*)wm_alloc(MAX_CONSTRUCTS_COUNT * sizeof(WarLevelConstruct));
    if (!levelInfoRes->levelInfo.startRoads)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for startRoads in '%.*s'",
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        SDL_CloseIO(stream);
        return false;
    }
    WFILE_READ_ARRAY(levelInfoRes->levelInfo.startRoads, sizeof(WarLevelConstruct), levelInfoRes->levelInfo.startRoadsCount);

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startWallsCount, sizeof(u32));
    if (levelInfoRes->levelInfo.startWallsCount > MAX_CONSTRUCTS_COUNT)
    {
        logError("wfile_loadWarMapFile: startWallsCount %u too large in '%.*s'",
                 levelInfoRes->levelInfo.startWallsCount,
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        SDL_CloseIO(stream);
        return false;
    }
    levelInfoRes->levelInfo.startWalls = (WarLevelConstruct*)wm_alloc(MAX_CONSTRUCTS_COUNT * sizeof(WarLevelConstruct));
    if (!levelInfoRes->levelInfo.startWalls)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for startWalls in '%.*s'",
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        SDL_CloseIO(stream);
        return false;
    }
    WFILE_READ_ARRAY(levelInfoRes->levelInfo.startWalls, sizeof(WarLevelConstruct), levelInfoRes->levelInfo.startWallsCount);

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startGoldminesCount, sizeof(u32));
    if (levelInfoRes->levelInfo.startGoldminesCount > MAX_CUSTOM_MAP_GOLDMINES_COUNT)
    {
        logError("wfile_loadWarMapFile: startGoldminesCount %u too large in '%.*s'",
                 levelInfoRes->levelInfo.startGoldminesCount,
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        wm_free(levelInfoRes->levelInfo.startWalls);
        SDL_CloseIO(stream);
        return false;
    }
    levelInfoRes->levelInfo.startGoldmines = (WarLevelUnit*)wm_alloc(MAX_CUSTOM_MAP_GOLDMINES_COUNT * sizeof(WarLevelUnit));
    if (!levelInfoRes->levelInfo.startGoldmines)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for startGoldmines in '%.*s'",
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        wm_free(levelInfoRes->levelInfo.startWalls);
        SDL_CloseIO(stream);
        return false;
    }
    WFILE_READ_ARRAY(levelInfoRes->levelInfo.startGoldmines, sizeof(WarLevelUnit), levelInfoRes->levelInfo.startGoldminesCount);

    WFILE_READ_FIELD(&levelInfoRes->levelInfo.startConfigurationsCount, sizeof(u32));
    if (levelInfoRes->levelInfo.startConfigurationsCount > MAX_CUSTOM_MAP_CONFIGURATIONS_COUNT)
    {
        logError("wfile_loadWarMapFile: startConfigurationsCount %u too large in '%.*s'",
                 levelInfoRes->levelInfo.startConfigurationsCount,
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        wm_free(levelInfoRes->levelInfo.startWalls);
        wm_free(levelInfoRes->levelInfo.startGoldmines);
        SDL_CloseIO(stream);
        return false;
    }

    // v2 format: startConfigurations serialized element-by-element
    // (u32 startEntitiesCount followed by N x WarLevelUnit). Read each
    // configuration and allocate the entity arrays accordingly.
    for (u32 ci = 0; ci < levelInfoRes->levelInfo.startConfigurationsCount; ci++)
    {
        WFILE_READ_FIELD(&levelInfoRes->levelInfo.startConfigurations[ci].startEntitiesCount, sizeof(u32));
        if (levelInfoRes->levelInfo.startConfigurations[ci].startEntitiesCount > MAX_CUSTOM_MAP_ENTITIES_COUNT)
        {
            logError("wfile_loadWarMapFile: startConfigurations[%u].startEntitiesCount %u too large in '%.*s'",
                     ci, levelInfoRes->levelInfo.startConfigurations[ci].startEntitiesCount,
                     (int)filePath.length, filePath.data);
            wm_free(levelInfoRes->levelInfo.startEntities);
            wm_free(levelInfoRes->levelInfo.startRoads);
            wm_free(levelInfoRes->levelInfo.startWalls);
            wm_free(levelInfoRes->levelInfo.startGoldmines);
            for (u32 cj = 0; cj < ci; cj++)
                wm_free(levelInfoRes->levelInfo.startConfigurations[cj].startEntities);
            SDL_CloseIO(stream);
            return false;
        }

        levelInfoRes->levelInfo.startConfigurations[ci].startEntities =
            (WarLevelUnit*)wm_alloc(MAX_CUSTOM_MAP_ENTITIES_COUNT * sizeof(WarLevelUnit));
        if (!levelInfoRes->levelInfo.startConfigurations[ci].startEntities)
        {
            logError("wfile_loadWarMapFile: couldn't allocate memory for startConfigurations[%u].startEntities in '%.*s'",
                     ci, (int)filePath.length, filePath.data);
            wm_free(levelInfoRes->levelInfo.startEntities);
            wm_free(levelInfoRes->levelInfo.startRoads);
            wm_free(levelInfoRes->levelInfo.startWalls);
            wm_free(levelInfoRes->levelInfo.startGoldmines);
            for (u32 cj = 0; cj < ci; cj++)
                wm_free(levelInfoRes->levelInfo.startConfigurations[cj].startEntities);
            SDL_CloseIO(stream);
            return false;
        }

        WFILE_READ_ARRAY(levelInfoRes->levelInfo.startConfigurations[ci].startEntities,
                        sizeof(WarLevelUnit),
                        levelInfoRes->levelInfo.startConfigurations[ci].startEntitiesCount);
    }

    visualInfoRes->levelVisual.data = (u16*)wm_alloc(MAP_TILES_WIDTH * MAP_TILES_HEIGHT * sizeof(u16));
    if (!visualInfoRes->levelVisual.data)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for levelVisual in '%.*s'",
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        wm_free(levelInfoRes->levelInfo.startWalls);
        wm_free(levelInfoRes->levelInfo.startGoldmines);
        SDL_CloseIO(stream);
        return false;
    }

    passableInfoRes->levelPassable.data = (u16*)wm_alloc(MAP_TILES_WIDTH * MAP_TILES_HEIGHT * sizeof(u16));
    if (!passableInfoRes->levelPassable.data)
    {
        logError("wfile_loadWarMapFile: couldn't allocate memory for levelPassable in '%.*s'",
                 (int)filePath.length, filePath.data);
        wm_free(levelInfoRes->levelInfo.startEntities);
        wm_free(levelInfoRes->levelInfo.startRoads);
        wm_free(levelInfoRes->levelInfo.startWalls);
        wm_free(levelInfoRes->levelInfo.startGoldmines);
        wm_free(visualInfoRes->levelVisual.data);
        SDL_CloseIO(stream);
        return false;
    }

    WFILE_READ_ARRAY(visualInfoRes->levelVisual.data,     sizeof(u16), MAP_TILES_WIDTH * MAP_TILES_HEIGHT);
    WFILE_READ_ARRAY(passableInfoRes->levelPassable.data, sizeof(u16), MAP_TILES_WIDTH * MAP_TILES_HEIGHT);

    SDL_CloseIO(stream);

    return true;

#undef WFILE_READ_FIELD
#undef WFILE_READ_ARRAY
}
