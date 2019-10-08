#define PHOTON_RADIUS 0.05
#define PHOTON_GRID_SIZE 128
#define PHOTON_CELL_SIZE (1.0/(PHOTON_GRID_SIZE))

#define PHOTONS_LOD	10

#define PHOTON_DIMENSION (1 << PHOTONS_LOD)
#define SHADOWMAP_DIMENSION (PHOTON_DIMENSION > 2048 ? PHOTON_DIMENSION : 2048)
#define PHOTON_TRACE_MAX_BOUNCES 2
#define RAY_TRACING_MAX_BOUNCES 1
#define PATH_TRACING_MAX_BOUNCES 2
#define LIGHT_SOURCE_RADIUS 0.04
#define HASH_CAPACITY (PHOTON_GRID_SIZE * PHOTON_GRID_SIZE + 1)
#define DESIRED_PHOTONS (PHOTON_DIMENSION / 2)

#define CS_1D_GROUPSIZE 1024
#define CS_2D_GROUPSIZE 32

#define BOXED_PHOTONS_LOD 2
#define BOXED_PHOTONS ((1 << BOXED_PHOTONS_LOD)*(1 << BOXED_PHOTONS_LOD))

#define COMPACT_PAYLOAD

//#define DEBUG_PHOTONS
#define DEBUG_STRATEGY 1

#define ADAPTIVE_POWER 0.5
#define ADAPTIVE_STRATEGY 0

#define USE_VOLUME_GRID true