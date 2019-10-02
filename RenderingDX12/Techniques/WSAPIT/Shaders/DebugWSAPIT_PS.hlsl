
struct PS_IN {
    float4 Projected    : SV_POSITION;
    float3 Position     : POSITION;
};

StructuredBuffer<int3> sceneBoundaries : register(t0);

float4 main(PS_IN input) : SV_TARGET
{
    //return float4(1, 0, 0, 1);

    float3 lowerCorner = sceneBoundaries[0] / 1e7;
    float3 upperCorner = sceneBoundaries[1] / 1e7;
    float unit = length(upperCorner - lowerCorner);

    float d = length(input.Position - lowerCorner);
    
    return float4(d / unit, 0, 0, 1);
}