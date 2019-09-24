#define PHOTON_RADIUS 0.1
#define PHOTON_GRID_SIZE 128
#define PHOTON_CELL_SIZE (1.0/(PHOTON_GRID_SIZE))

#define PHOTONS_LOD 9

#define PHOTON_DIMENSION (1 << PHOTONS_LOD)
#define SHADOWMAP_DIMENSION (PHOTON_DIMENSION > 2048 ? PHOTON_DIMENSION : 2048)
#define PHOTON_TRACE_MAX_BOUNCES 1
#define RAY_TRACING_MAX_BOUNCES 1
#define PATH_TRACING_MAX_BOUNCES 3
#define LIGHT_SOURCE_RADIUS 0.1
#define HASH_CAPACITY (PHOTON_GRID_SIZE * PHOTON_GRID_SIZE + 1)
#define DESIRED_PHOTONS 100

//#define DEBUG_PHOTONS
#define DEBUG_STRATEGY 0

#define ADAPTIVE_POWER 0.5
#define ADAPTIVE_STRATEGY 0

#define USE_VOLUME_GRID true