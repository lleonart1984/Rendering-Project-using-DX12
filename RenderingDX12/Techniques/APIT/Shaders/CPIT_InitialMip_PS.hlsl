Texture2D<int> rootBuffer : register(t0); // first buffer updated to be tree root nodes...
StructuredBuffer<float4> boundaryBuffer : register(t1);
StructuredBuffer<int> Morton : register(t2);

RWStructuredBuffer<float2> MipMaps : register(u0);

int get_index(int2 px)
{
    int width, height;
    rootBuffer.GetDimensions(width, height);

    int view = px.x / height;

    return Morton[px.x % height] + Morton[px.y] * 2 + view * height * height;
}

void main(float4 proj: SV_POSITION)
{
    int2 px = int2(proj.xy);
    MipMaps[get_index(px)] = boundaryBuffer[rootBuffer[px]].zw;
}