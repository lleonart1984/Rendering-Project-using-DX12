#ifndef COMMONOUTPUT_H
#define COMMONOUTPUT_H

/// Common shader header to represent this library will produce an output image
// Requires an read-write texture uav at OUTPUT_IMAGE_REG register

// Raytracing output image
RWTexture2D<float3> Output				: register(OUTPUT_IMAGE_REG);

#endif