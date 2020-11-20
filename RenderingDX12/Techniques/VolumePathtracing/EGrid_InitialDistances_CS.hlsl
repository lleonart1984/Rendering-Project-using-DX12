/// GridInitialDistances_CS

#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> OB : register(t1); // Object buffers
StructuredBuffer<float4x4> Transforms : register(t2); // World transforms
StructuredBuffer<int> TriangleIndices : register(t3); // Linked list values (references to the triangles)
Texture3D<int> Head : register(t4); // Per cell linked list head
StructuredBuffer<int> Next : register(t5); // Per linked lists next references....

RWTexture3D<float> DistanceField : register(u0);

cbuffer GridInfo : register(b0) {
	int Size;
	float3 Min;
	float3 Max;
}

/// Point to Point
float Distance(float3 p1, float3 p2) {
	return length(p1 - p2);
}

/// Point to segment
float Distance(float3 p, float3 s[2]) {
	float3 d = s[1] - s[0];
	float alpha = clamp(dot(p - s[0], d) / dot(d, d), 0, 1);
	return Distance(p, lerp(s[0], s[1], alpha));
}

float3 Project(float3 p, float3 P, float3 N) {
	return p - N * dot(p - P, N);
}

/// Point to Triangle
float Distance(float3 p, float3 t[3]) {
	float3 N = normalize(cross(t[2] - t[0], t[1] - t[0]));
	float3 P = t[0];

	float3 x = Project(p, P, N);

	float3x3 M = float3x3(
		cross(t[1] - t[2], x - t[2]),
		cross(t[2] - t[0], x - t[0]),
		cross(t[0] - t[1], x - t[1])
		);

	float3 bary = mul(N, transpose(M));
	bary /= (bary.x + bary.y + bary.z);
	bary = clamp(bary, 0, 1);
	bary /= (bary.x + bary.y + bary.z);
	return Distance(p, bary.x * t[0] + bary.y * t[1] + bary.z * t[2]);
}

/// Segment to Segment
float Distance(float3 s1[2], float3 s2[2]) {
	float3   u = s1[1] - s1[0];
	float3   v = s2[1] - s2[0];
	float3   w = s1[0] - s2[0];
	float    a = dot(u, u);         // always >= 0
	float    b = dot(u, v);
	float    c = dot(v, v);         // always >= 0
	float    d = dot(u, w);
	float    e = dot(v, w);
	float    D = a * c - b * b;        // always >= 0
	float    sc, sN, sD = D;       // sc = sN / sD, default sD = D >= 0
	float    tc, tN, tD = D;       // tc = tN / tD, default tD = D >= 0

	// compute the line parameters of the two closest points
	if (D < 0.00001) { // the lines are almost parallel
		sN = 0.0;         // force using point P0 on segment S1
		sD = 1.0;         // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	}
	else {                 // get the closest points on the infinite lines
		sN = (b * e - c * d);
		tN = (a * e - b * d);
		if (sN < 0.0) {        // sc < 0 => the s=0 edge is visible
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD) {  // sc > 1  => the s=1 edge is visible
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0) {            // tc < 0 => the t=0 edge is visible
		tN = 0.0;
		// recompute sc for this edge
		if (-d < 0.0)
			sN = 0.0;
		else if (-d > a)
			sN = sD;
		else {
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD) {      // tc > 1  => the t=1 edge is visible
		tN = tD;
		// recompute sc for this edge
		if ((-d + b) < 0.0)
			sN = 0;
		else if ((-d + b) > a)
			sN = sD;
		else {
			sN = (-d + b);
			sD = a;
		}
	}
	// finally do the division to get sc and tc
	sc = (abs(sN) < 0.00001 ? 0.0 : sN / sD);
	tc = (abs(tN) < 0.00001 ? 0.0 : tN / tD);

	// get the difference of the two closest points
	float3   dP = w + (sc * u) - (tc * v);  // =  S1(sc) - S2(tc)

	return length(dP);   // return the closest distance
}


/// Segment to Triangle
float Distance(float3 s[2], float3 t[3]) {
	float3 N = normalize(cross(t[2] - t[0], t[1] - t[0]));
	float3 P = t[0];

	float alpha = (dot(P, N) - dot(s[0], N)) / dot(s[1] - s[0], N);

	float3 closestPointInSegment = lerp(s[0], s[1], alpha);

	return Distance(closestPointInSegment, t);
}

void ClipPositive(float3 P, float3 N, inout float3 e0, inout float3 e1) {
	float d0 = dot(e0 - P, N);
	float d1 = dot(e1 - P, N);
	if (d0 < 0) // clip e0
	{
		e0 = lerp(e0, e1, -d0 / (d1 - d0));
		d0 = 0;
	}

	if (d1 < 0) // clip e1
	{
		e1 = lerp(e1, e0, -d1 / (d0 - d1));
		d1 = 0;
	}
}

/// Distance from Side (Corner C, Up U, Right R, Normal N) to Triangle
float Distance(float3 C, float3 U, float3 R, float3 N, float3 t[3]) {
	float3 p00 = C;
	float3 p01 = C + R;
	float3 p10 = C + U;
	float3 p11 = C + U + R;

	float dist = 1000000;

	for (int e = 0; e < 3; e++)
	{
		float3 e0 = t[e];
		float3 e1 = t[(e + 1) % 3];

		ClipPositive(p00, R, e0, e1);
		ClipPositive(p00, U, e0, e1);
		ClipPositive(p01, -R, e0, e1);
		ClipPositive(p10, -U, e0, e1);

		if (any(e0 - e1)) {
			dist = min(dist, dot(e0 - C, N));
			dist = min(dist, dot(e1 - C, N));
		}
	}

	return dist;
}

/// Gets the triangle in Grid space (0,0,0)-(Size, Size, Size)
void GetTriangle(int triangleIndex, inout float3 t[3]) {
	float4x4 world = Transforms[OB[triangleIndex * 3]];

	t[0] = (mul(vertices[triangleIndex * 3 + 0].P, world) - Min) * Size / (Max - Min);
	t[2] = (mul(vertices[triangleIndex * 3 + 2].P, world) - Min) * Size / (Max - Min);
	t[1] = (mul(vertices[triangleIndex * 3 + 1].P, world) - Min) * Size / (Max - Min);
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int3 currentCell = int3(DTid.x % Size, DTid.x / Size % Size, DTid.x / (Size * Size));

	if (Head[currentCell] != -1) // not empty cell
	{
		DistanceField[currentCell] = -1;
		return;
	}

	float3 corners[2][2][2];
	for (int cz = 0; cz < 2; cz++)
		for (int cy = 0; cy < 2; cy++)
			for (int cx = 0; cx < 2; cx++)
				corners[cx][cy][cz] = currentCell + float3(cx, cy, cz);


	float dist = 0.99999;

	for (int bz = -1; bz <= 1; bz++)
		for (int by = -1; by <= 1; by++)
			for (int bx = -1; bx <= 1; bx++)
			{
				int3 b = int3(bx, by, bz);

				int3 adjCell = clamp(currentCell + b, 0, Size - 1);

				int type = abs(bz) + abs(by) + abs(bx);

				if (type == 3) // corners
				{
					int currentTriangle = Head[adjCell];
					while (currentTriangle != -1) {

						float3 t[3];
						GetTriangle(currentTriangle, t);

						dist = min(dist, Distance(corners[(bx + 1) / 2][(by + 1) / 2][(bz + 1) / 2], t));

						currentTriangle = Next[currentTriangle];
					}
				}
				if (type == 2) // edges (bx == 0 || by == 0 || bz == 0)
				{
					int3 planeAxis = 1 - abs(b);
					int3 mask = abs(b);

					int3 coord0 = (b + 1) / 2;
					int3 coord1 = coord0 * mask + planeAxis;

					float3 edge[2] = { corners[coord0.x][coord0.y][coord0.z], corners[coord1.x][coord1.y][coord1.z] };

					int currentTriangle = Head[adjCell];
					while (currentTriangle != -1) {

						float3 t[3];
						GetTriangle(currentTriangle, t);

						dist = min(dist, Distance(edge, t));

						currentTriangle = Next[currentTriangle];
					}
				}
				if (type == 1)
				{
					float3 N = b;
					float3 C = currentCell + ((b + 1) / 2); // intentionally int division
					float3 B = abs(bz) == 1 ? float3(1, 0, 0) : float3(0, 0, 1);
					float3 T = abs(cross(B, N)); // TODO: improve this!

					int currentTriangle = Head[adjCell];
					while (currentTriangle != -1) {

						float3 t[3];
						GetTriangle(currentTriangle, t);

						dist = min(dist, Distance(C, B, T, N, t));

						currentTriangle = Next[currentTriangle];
					}
				}
			}

	DistanceField[currentCell] = dist; // Value between 0..0.99999 indicating the safe distance in a cell regarding the adjacents.
}
