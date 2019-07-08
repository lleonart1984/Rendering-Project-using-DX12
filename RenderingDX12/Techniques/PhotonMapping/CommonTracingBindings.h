
#include "../CommonRT/CommonRTScenes.h"

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(GBUFFER_POSITIONS_REG);
Texture2D<float3> Normals				: register(GBUFFER_NORMALS_REG);
Texture2D<float2> Coordinates			: register(GBUFFER_COORDINATES_REG);
Texture2D<int> MaterialIndices			: register(GBUFFER_MATERIALS_REG);

