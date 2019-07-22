#define GRID_HASH_TABLE_REG t8
#define LINKED_LIST_NEXT_REG t9
#define PHOTON_BUFFER_REG t10
// Override TEXTURES_REG
#define TEXTURES_REG t11

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../CommonGatheringSpatialHashGrid.hlsl.h"
#include "GPMCommon.hlsl.h" // Implements GetHashIndex method as a fixed bounded grid
