#define PHOTON_RADIUS 0.025
#define PHOTON_GRID_SIZE 128
#define PHOTON_CELL_SIZE (1.0/(PHOTON_GRID_SIZE))
#define PHOTON_DIMENSION 2048
#define SHADOWMAP_DIMENSION 2048
#define PHOTON_TRACE_MAX_BOUNCES 2
#define RAY_TRACING_MAX_BOUNCES 2
#define PATH_TRACING_MAX_BOUNCES 2
#define LIGHT_SOURCE_RADIUS 0.1
#define HASH_CAPACITY (PHOTON_GRID_SIZE * PHOTON_GRID_SIZE + 1)

//#define DEBUG_PHOTONS

#define ADAPTIVE_POWER 0.5
#define ADAPTIVE_STRATEGY 1
