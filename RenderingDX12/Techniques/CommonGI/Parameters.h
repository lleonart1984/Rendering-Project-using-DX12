#define PHOTON_RADIUS 0.01
#define PHOTON_GRID_SIZE 128
#define PHOTON_CELL_SIZE (1.0/(PHOTON_GRID_SIZE))
#define PHOTON_DIMENSION 1024
#define SHADOWMAP_DIMENSION 1024
#define PHOTON_TRACE_MAX_BOUNCES 1
#define RAY_TRACING_MAX_BOUNCES 1
#define PATH_TRACING_MAX_BOUNCES 2
#define LIGHT_SOURCE_RADIUS 0.05
#define HASH_CAPACITY (PHOTON_GRID_SIZE * PHOTON_GRID_SIZE + 1)
#define DESIRED_PHOTONS 50

#define DEBUG_PHOTONS
#define DEBUG_STRATEGY 1

#define ADAPTIVE_POWER 0.5
#define ADAPTIVE_STRATEGY 1

#define USE_VOLUME_GRID true