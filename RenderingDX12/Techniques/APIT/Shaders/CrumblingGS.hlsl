struct GS_IN
{
    float3 P : POSITION;
    float3 N : NORMAL;
    float2 C : TEXCOORD;
};

struct GS_OUT
{
    float4 proj : SV_POSITION;

    float3 P1 : POSITION0;
    float3 N1 : NORMAL0;
    float2 C1 : TEXCOORD0;
    float3 P2 : POSITION1;
    float3 N2 : NORMAL1;
    float2 C2 : TEXCOORD1;
    float3 P3 : POSITION2;
    float3 N3 : NORMAL2;
    float2 C3 : TEXCOORD2;
};

float ceilDiv(float a, float b)
{
    if (a == 1 || a == -1) return a;

    return ceil(a / b) * b;
}

float3 PerspectiveRound(float3 position)
{
    return position;

    if (!any(position))
        return position;

    float depth = max(max(abs(position.x), abs(position.y)), abs(position.z));

    float3 projected = position / depth;

    float projectedPixelSize = 2 / 512.0;

    projected = float3 (ceilDiv(projected.x, projectedPixelSize) + projectedPixelSize / 2, ceilDiv(projected.y, projectedPixelSize) + projectedPixelSize / 2, ceilDiv(projected.z, projectedPixelSize) + projectedPixelSize / 2);

    float power = ceil(log(depth) / log(1 + projectedPixelSize));

    depth = pow(1 + projectedPixelSize, power);

    return projected * depth;
}

[maxvertexcount(3)]
void GS(triangle GS_IN vertexes[3], inout PointStream<GS_OUT> triangleOut)
{
    GS_OUT Out = (GS_OUT)0;

    Out.proj = float4(0, 0, 0.5, 1);

    Out.P1 = PerspectiveRound(vertexes[0].P);
    Out.N1 = vertexes[0].N;
    Out.C1 = vertexes[0].C;
    Out.P2 = PerspectiveRound(vertexes[1].P);
    Out.N2 = vertexes[1].N;
    Out.C2 = vertexes[1].C;
    Out.P3 = PerspectiveRound(vertexes[2].P);
    Out.N3 = vertexes[2].N;
    Out.C3 = vertexes[2].C;

    //if (any (Out.P1 - Out.P2) && any (Out.P2 - Out.P3) && any (Out.P2 - Out.P3))
    triangleOut.Append(Out);
}
