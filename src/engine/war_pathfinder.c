#include <assert.h>

#include "war_alloc.h"
#include "war_pathfinder.h"
#include "war_collections.h"

static const s32 wpath_dirC = 8;
static const s32 wpath_dirX[] = {  0,  1, 1, 1, 0, -1, -1, -1 };
static const s32 wpath_dirY[] = { -1, -1, 0, 1, 1,  1,  0, -1 };

typedef struct _WarMapNode
{
    s32 id;     // id of the node
    s32 x, y;   // the coordinates of the node
    s32 level;  // the length of the path from the start to this node
    s32 parent; // the previous node in the path from start to end passing through this node
    s32 gScore; // the cost from the start to this node
    s32 fScore; // the cost from start to end passing through this node
} WarMapNode;

shlDeclareList(WarMapNodeList, WarMapNode)
shlDefineList(WarMapNodeList, WarMapNode)

shlDeclareBinaryHeap(WarMapNodeHeap, WarMapNode)
shlDefineBinaryHeap(WarMapNodeHeap, WarMapNode)

shlDeclareMap(WarMapNodeMap, s32, WarMapNode)
shlDefineMap(WarMapNodeMap, s32, WarMapNode)

#include "port/pathfinder.inc"

void wpath_initPathFinderAt(WarPathFinder* finder, u16 data[MAP_TILES_WIDTH * MAP_TILES_HEIGHT])
{
    memset(finder, 0, sizeof(*finder));
    for(s32 i = 0; i < MAP_TILES_WIDTH * MAP_TILES_HEIGHT; i++)
        finder->data[i] = (data[i] == 128 || data[i] == 64) ? PATH_FINDER_DATA_STATIC : PATH_FINDER_DATA_EMPTY;
}

bool wpath_isInside(s32 x, s32 y)
{
    return inRange(x, 0, MAP_TILES_WIDTH) && inRange(y, 0, MAP_TILES_HEIGHT);
}

bool wpath_isTileAccesible(WarPathFinder* finder, s32 x, s32 y)
{
    for(s32 d = 0; d < wpath_dirC; d++)
    {
        s32 xx = x + wpath_dirX[d];
        s32 yy = y + wpath_dirY[d];
        if (inRange(xx, 0, MAP_TILES_WIDTH) && inRange(yy, 0, MAP_TILES_HEIGHT))
        {
            if (wpath_isEmpty(finder, xx, yy))
                return true;
        }
    }

    return false;
}

u16 wpath_getTileValue(WarPathFinder* finder, s32 x, s32 y)
{
    assert(inRange(x, 0, MAP_TILES_WIDTH));
    assert(inRange(y, 0, MAP_TILES_HEIGHT));
    return finder->data[y * MAP_TILES_WIDTH + x];
}

void wpath_setTilesValue(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height, u16 value)
{
    assert(finder);

    if (!inRange(startX, 0, MAP_TILES_WIDTH) || !inRange(startY, 0, MAP_TILES_HEIGHT))
        return;

    if (startX + width >= MAP_TILES_WIDTH)
        width = MAP_TILES_WIDTH - startX;

    if (startY + height >= MAP_TILES_HEIGHT)
        height = MAP_TILES_HEIGHT - startY;

    s32 endX = startX + width;
    s32 endY = startY + height;

    for(s32 y = startY; y < endY; y++)
    {
        for(s32 x = startX; x < endX; x++)
        {
            finder->data[y * MAP_TILES_WIDTH + x] = value;
        }
    }
}

void wpath_setFreeTiles(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height)
{
    wpath_setTilesValue(finder, startX, startY, width, height, PATH_FINDER_DATA_EMPTY);
}

void wpath_setStaticEntity(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height, WarEntityId id)
{
    wpath_setTilesValue(finder, startX, startY, width, height, (id << 4) | PATH_FINDER_DATA_STATIC);
}

WarPathFinderDataType wpath_getTileValueType(WarPathFinder* finder, s32 x, s32 y)
{
    return (WarPathFinderDataType)(wpath_getTileValue(finder, x, y) & 0x000F);
}

WarEntityId wpath_getTileEntityId(WarPathFinder* finder, s32 x, s32 y)
{
    return (WarEntityId)((wpath_getTileValue(finder, x, y) & 0xFFF0) >> 4);
}

bool wpath_isEmpty(WarPathFinder* finder, s32 x, s32 y)
{
    return wpath_getTileValueType(finder, x, y) == PATH_FINDER_DATA_EMPTY;
}

bool wpath_isStatic(WarPathFinder* finder, s32 x, s32 y)
{
    return wpath_getTileValueType(finder, x, y) == PATH_FINDER_DATA_STATIC;
}
