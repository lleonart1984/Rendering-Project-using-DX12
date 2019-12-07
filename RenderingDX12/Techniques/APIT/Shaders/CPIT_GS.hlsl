cbuffer ScreenInfo : register (b0) {
    int Width;
    int Height;

    int Levels;
}

StructuredBuffer<float3x3> LayerTransforms : register(t0);


struct GS_IN {
    float3 P : POSITION;
    uint VertexIndex : VERTEXINDEX;
};

struct GS_OUT {
    float4 proj : SV_POSITION;
    float3 P0 : POSITION0;
    float3 P1 : POSITION1;
    float3 P2 : POSITION2;
    int TriangleIndex : TRIANGLEINDEX;
    int ViewIndex : VIEWINDEX;
};

void updateCorners(float3 proj, inout float2 minim, inout float2 maxim)
{
    float2 screen = proj.xy / proj.z;
    minim = min(minim, screen);
    maxim = max(maxim, screen);
}

void AddToView(inout TriangleStream<GS_OUT> output, GS_IN input[3], int view, float3x3 faceTransform)
{
    GS_OUT clone = (GS_OUT)0;
    clone.TriangleIndex = input[0].VertexIndex / 3;
    clone.ViewIndex = view;

    float3 projected[3];
    for (int v = 0; v < 3; v++)
        projected[v] = mul(input[v].P, faceTransform);

    clone.P0 = projected[0];
    clone.P1 = projected[1];
    clone.P2 = projected[2];

    float2 dimensions = float2(Width, Height);

    float2 minim = float2(1, 1);
    float2 maxim = float2(-1, -1);

    for (int i = 0; i < 3; i++)
    {
        float iDz = 0.001 - projected[i].z;
        if (iDz <= 0)
        {
            updateCorners(projected[i], minim, maxim);
            for (int j = 0; j < 3; j++)
                if (projected[j].z < 0.001) // Perform Clipping if necessary
                {
                    float alpha = iDz / (projected[j].z - projected[i].z);
                    updateCorners(lerp(projected[i], projected[j], alpha), minim, maxim);
                }
        }
    }

    //if (all(maxim - minim < 0.001))
    //    return;

    if (all(maxim - int2(-1, -1)) && all(minim - int2(1, 1))) // some posible point inside view
    {
        minim -= float2(1, 1) / dimensions; // same that 2*float2(0.5,0.5)/dimensions;
        maxim += float2(1, 1) / dimensions;

        minim = max(float2(-1, -1), minim);
        maxim = min(float2(1, 1), maxim);

        clone.proj = float4(minim.x, minim.y, 0.5, 1);
        output.Append(clone);

        clone.proj = float4(maxim.x, minim.y, 0.5, 1);
        output.Append(clone);

        clone.proj = float4(minim.x, maxim.y, 0.5, 1);
        output.Append(clone);

        clone.proj = float4(maxim.x, maxim.y, 0.5, 1);
        output.Append(clone);

        output.RestartStrip();
    }
}

[maxvertexcount(24)]
void main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> output)
{
    for (int face = 0; face < 6; face++) {
        AddToView(output, input, face, LayerTransforms[face]);
    }
}