//typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
	float4 color;
};

struct MyAttributes {
	float2 b;
};

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

[shader("raygeneration")]
void MyRaygenShader()
{
	float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

	// Orthographic projection since we're raytracing in screen space.
	float3 rayDir = float3(0.000001, 0.000001, 1);// normalize(float3(lerpValues * 2 - 1, 1));
	//float3 origin = float3(0, 0, -1);
	float3 origin = float3(lerpValues * 2 - 1, -2);

	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	// Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
	// TMin should be kept small to prevent missing geometry at close contact areas.
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	RayPayload payload = { float4(0, 0, 0, 1) };
	TraceRay(Scene, RAY_FLAG_FORCE_NON_OPAQUE, ~0, 0, 0, 0, ray, payload);

	// Write the raytraced color to the output texture.
	RenderTarget[DispatchRaysIndex().xy] = payload.color;// float4(lerpValues, 0, 1);// payload.color;
}

[shader("anyhit")]
void MyAnyHit(inout RayPayload payload, in MyAttributes attr) {
	
	//if ((int)PrimitiveIndex() == -1)
		payload.color += float4(0.01, 1, 0.01, 0);
	//else {
	//	int f = abs((int)PrimitiveIndex()) / (float)(128 * 128);
	//	payload.color += float4(0.01, f, 0.01, 0);
	//}
	IgnoreHit();
}

[shader("intersection")]
void MyIntersectionShader() {
	float THit = RayTCurrent();
	MyAttributes attr = (MyAttributes)float2(RayTCurrent(), THit/20000.0);

	if((int)PrimitiveIndex() <= 10000)
		ReportHit(0.5, /*hitKind*/ 0, attr);
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
	//float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	//payload.color = float4(0, 0, 1, 1);// float4(barycentrics, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
	//payload.color = float4(1, 0, 1, 1);
}

