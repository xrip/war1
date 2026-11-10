#pragma once

#include "shl/binary_heap.h"
#include "shl/list.h"
#include "shl/map.h"

#include "core/war.h"
#include "core/common.h"
#include "core/fwd.h"
#include "core/math.h"
#include "core/collections.h"

struct _WarMapPath
{
    vec2 nodes[MAP_TILES_WIDTH * MAP_TILES_HEIGHT];
    s32 count;
};

// only the directions are ever read
struct _WarMapFlowField
{
    u8 dirs[MAP_TILES_WIDTH * MAP_TILES_HEIGHT];
};

struct _WarPathFinder
{
    u16 data[MAP_TILES_WIDTH * MAP_TILES_HEIGHT];
    WarMapFlowField* fields[MAP_TILES_WIDTH * MAP_TILES_HEIGHT];
};

// the same, in place (the struct is 24 KB: returned by value it was
// built on the board's 32 KB stack, and overflowed it)
void wpath_initPathFinderAt(WarPathFinder* finder, u16 data[MAP_TILES_WIDTH * MAP_TILES_HEIGHT]);
bool wpath_isInside(s32 x, s32 y);
void wpath_flowField(WarPathFinder* finder, s32 x, s32 y, WarMapFlowField* flowField);
WarMapFlowField* wpath_computeFlowField(WarPathFinder* finder, s32 x, s32 y);
void wpath_freeFlowFields(WarPathFinder* finder);
vec2 wpath_findEmptyTile(WarPathFinder* finder, s32 x, s32 y);
bool wpath_isTileAccesible(WarPathFinder* finder, s32 x, s32 y);
u16 wpath_getTileValue(WarPathFinder* finder, s32 x, s32 y);
WarPathFinderDataType wpath_getTileValueType(WarPathFinder* finder, s32 x, s32 y);
WarEntityId wpath_getTileEntityId(WarPathFinder* finder, s32 x, s32 y);
void wpath_setTilesValue(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height, u16 value);
void wpath_setFreeTiles(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height);
void wpath_setStaticEntity(WarPathFinder* finder, s32 startX, s32 startY, s32 width, s32 height, WarEntityId id);
bool wpath_isEmpty(WarPathFinder* finder, s32 x, s32 y);
bool wpath_isStatic(WarPathFinder* finder, s32 x, s32 y);
