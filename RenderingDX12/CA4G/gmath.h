#ifndef GMATH_H
#define GMATH_H

#include <cmath>

#define PI (float)3.14159265358979323846

#define O_4x4 float4x4(0)
#define I_4x4 float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

#define xyz(v) float3(v.x,v.y,v.z)

typedef struct ARGB
{
	unsigned int value;

	int Alpha() { return value >> 24; }
	int Red() { return (value & 0xFF0000) >> 16; }
	int Green() { return (value & 0xFF00) >> 8; }
	int Blue() { return (value & 0xFF); }

	ARGB() { value = 0; }

	ARGB(int alpha, int red, int green, int blue) {
		value =
			((unsigned int)alpha) << 24 |
			((unsigned int)red) << 16 |
			((unsigned int)green) << 8 |
			((unsigned int)blue);
	}
} ARGB;

typedef struct RGBA
{
	unsigned int value;

	int Alpha() { return value >> 24; }
	int Blue() { return (value & 0xFF0000) >> 16; }
	int Green() { return (value & 0xFF00) >> 8; }
	int Red() { return (value & 0xFF); }

	RGBA() { value = 0; }

	RGBA(int alpha, int red, int green, int blue) {
		value =
			((unsigned int)alpha) << 24 |
			((unsigned int)blue) << 16 |
			((unsigned int)green) << 8 |
			((unsigned int)red);
	}
} RGBA;

typedef struct int2 {
	int x, y;

	int2(int x, int y) :x(x), y(y) {
	}
	int2(int v) : int2(v, v) {}
	int2() :int2(0) {}
} int2;

typedef struct int3 {
	int x, y, z;

	int3(int x, int y, int z) :x(x), y(y), z(z) {
	}
	int3(int v) : int3(v, v, v) {}
	int3() :int3(0) {}
} int3;

typedef struct int4 {
	int x, y, z, w;

	int4(int x, int y, int z, int w) :x(x), y(y), z(z), w(w) {
	}
	int4(int v) : int4(v, v, v, v) {}
	int4() : int4(0) {}
} int4;


typedef struct uint2 {
	unsigned int x, y;

	uint2(unsigned int x, unsigned int y) :x(x), y(y) {
	}
	uint2(unsigned int v) :uint2(v, v) {}
	uint2() :uint2(0) {}
} uint2;

typedef struct uint3 {
	unsigned int x, y, z;

	uint3(unsigned int x, unsigned int y, unsigned int z) :x(x), y(y), z(z) {
	}
	uint3(unsigned int v) :uint3(v, v, v) {}
	uint3() :uint3(0) {}
} uint3;

typedef struct uint4 {
	unsigned int x, y, z, w;

	uint4(unsigned int x, unsigned int y, unsigned int z, unsigned int w) :x(x), y(y), z(z), w(w) {
	}
	uint4(unsigned int v) :uint4(v, v, v, v) {}
	uint4() :uint4(0) {}
} uint4;

typedef struct float2
{
	float x, y;
	float2(float x, float y) :x(x), y(y) {}
	float2(float v) :float2(v, v) {}
	float2() :float2(0) {}
} float2;

typedef struct float3
{
	float x, y, z;
	float3(float x, float y, float z) :x(x), y(y), z(z) {}
	float3(float v) :float3(v, v, v) {}
	float3() :float3(0) {}
	float3(float2 v, float z) :float3(v.x, v.y, z) {}

	float2 getXY() { return float2(x, y); }
} float3;

typedef struct float4
{
	float x, y, z, w;
	float4(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {}
	float4(float v) :float4(v, v, v, v) {}
	float4() :float4(0) {}
	float4(float3 v, float w) :float4(v.x, v.y, v.z, w) {}

	float3 getXYZ() { return float3(x, y, z); }
	float3 getXYZHomogenized() { return float3(x / w, y / w, z / w); }
} float4;

typedef struct float2x2
{
	float M00;
	float M01;
	float M10;
	float M11;

	float2x2(float value)
	{
		M00 = value;
		M01 = value;
		M10 = value;
		M11 = value;
	}
	float2x2() :float2x2(0) {}
	float2x2(float M00, float M01, float M10, float M11)
	{
		this->M00 = M00;
		this->M01 = M01;
		this->M10 = M10;
		this->M11 = M11;
	}

} float2x2;

typedef struct float4x4
{
	float M00;
	float M01;
	float M02;
	float M03;
	float M10;
	float M11;
	float M12;
	float M13;
	float M20;
	float M21;
	float M22;
	float M23;
	float M30;
	float M31;
	float M32;
	float M33;
	float4x4(float value)
	{
		M00 = value;
		M01 = value;
		M02 = value;
		M03 = value;
		M10 = value;
		M11 = value;
		M12 = value;
		M13 = value;
		M20 = value;
		M21 = value;
		M22 = value;
		M23 = value;
		M30 = value;
		M31 = value;
		M32 = value;
		M33 = value;
	}
	float4x4() :float4x4(0) {}
	float4x4(float M00, float M01, float M02, float M03, float M10, float M11, float M12, float M13, float M20, float M21, float M22, float M23, float M30, float M31, float M32, float M33)
	{
		this->M00 = M00;
		this->M01 = M01;
		this->M02 = M02;
		this->M03 = M03;
		this->M10 = M10;
		this->M11 = M11;
		this->M12 = M12;
		this->M13 = M13;
		this->M20 = M20;
		this->M21 = M21;
		this->M22 = M22;
		this->M23 = M23;
		this->M30 = M30;
		this->M31 = M31;
		this->M32 = M32;
		this->M33 = M33;
	}
	float4x4 getInverse() const {
		float Min00 = M11 * M22 * M33 + M12 * M23 * M31 + M13 * M21 * M32 - M11 * M23 * M32 - M12 * M21 * M33 - M13 * M22 * M31;
		float Min01 = M10 * M22 * M33 + M12 * M23 * M30 + M13 * M20 * M32 - M10 * M23 * M32 - M12 * M20 * M33 - M13 * M22 * M30;
		float Min02 = M10 * M21 * M33 + M11 * M23 * M30 + M13 * M20 * M31 - M10 * M23 * M31 - M11 * M20 * M33 - M13 * M21 * M30;
		float Min03 = M10 * M21 * M32 + M11 * M22 * M30 + M12 * M20 * M31 - M10 * M22 * M31 - M11 * M20 * M32 - M12 * M21 * M30;

		float det = Min00 * M00 - Min01 * M01 + Min02 * M02 - Min03 * M03;

		if (det == 0)
			return float4x4(0);

		float Min10 = M01 * M22 * M33 + M02 * M23 * M31 + M03 * M21 * M32 - M01 * M23 * M32 - M02 * M21 * M33 - M03 * M22 * M31;
		float Min11 = M00 * M22 * M33 + M02 * M23 * M30 + M03 * M20 * M32 - M00 * M23 * M32 - M02 * M20 * M33 - M03 * M22 * M30;
		float Min12 = M00 * M21 * M33 + M01 * M23 * M30 + M03 * M20 * M31 - M00 * M23 * M31 - M01 * M20 * M33 - M03 * M21 * M30;
		float Min13 = M00 * M21 * M32 + M01 * M22 * M30 + M02 * M20 * M31 - M00 * M22 * M31 - M01 * M20 * M32 - M02 * M21 * M30;

		float Min20 = M01 * M12 * M33 + M02 * M13 * M31 + M03 * M11 * M32 - M01 * M13 * M32 - M02 * M11 * M33 - M03 * M12 * M31;
		float Min21 = M00 * M12 * M33 + M02 * M13 * M30 + M03 * M10 * M32 - M00 * M13 * M32 - M02 * M10 * M33 - M03 * M12 * M30;
		float Min22 = M00 * M11 * M33 + M01 * M13 * M30 + M03 * M10 * M31 - M00 * M13 * M31 - M01 * M10 * M33 - M03 * M11 * M30;
		float Min23 = M00 * M11 * M32 + M01 * M12 * M30 + M02 * M10 * M31 - M00 * M12 * M31 - M01 * M10 * M32 - M02 * M11 * M30;

		float Min30 = M01 * M12 * M23 + M02 * M13 * M21 + M03 * M11 * M22 - M01 * M13 * M22 - M02 * M11 * M23 - M03 * M12 * M21;
		float Min31 = M00 * M12 * M23 + M02 * M13 * M20 + M03 * M10 * M22 - M00 * M13 * M22 - M02 * M10 * M23 - M03 * M12 * M20;
		float Min32 = M00 * M11 * M23 + M01 * M13 * M20 + M03 * M10 * M21 - M00 * M13 * M21 - M01 * M10 * M23 - M03 * M11 * M20;
		float Min33 = M00 * M11 * M22 + M01 * M12 * M20 + M02 * M10 * M21 - M00 * M12 * M21 - M01 * M10 * M22 - M02 * M11 * M20;

		return float4x4(
			(+Min00 / det), (-Min10 / det), (+Min20 / det), (-Min30 / det),
			(-Min01 / det), (+Min11 / det), (-Min21 / det), (+Min31 / det),
			(+Min02 / det), (-Min12 / det), (+Min22 / det), (-Min32 / det),
			(-Min03 / det), (+Min13 / det), (-Min23 / det), (+Min33 / det));
	}
	float getDeterminant() const {
		float Min00 = M11 * M22 * M33 + M12 * M23 * M31 + M13 * M21 * M32 - M11 * M23 * M32 - M12 * M21 * M33 - M13 * M22 * M31;
		float Min01 = M10 * M22 * M33 + M12 * M23 * M30 + M13 * M20 * M32 - M10 * M23 * M32 - M12 * M20 * M33 - M13 * M22 * M30;
		float Min02 = M10 * M21 * M33 + M11 * M23 * M30 + M13 * M20 * M31 - M10 * M23 * M31 - M11 * M20 * M33 - M13 * M21 * M30;
		float Min03 = M10 * M21 * M32 + M11 * M22 * M30 + M12 * M20 * M31 - M10 * M22 * M31 - M11 * M20 * M32 - M12 * M21 * M30;

		return Min00 * M00 - Min01 * M01 + Min02 * M02 - Min03 * M03;
	}
	bool IsSingular() const { return getDeterminant() == 0; }
} float4x4;

typedef struct float3x3
{
	float M00;
	float M01;
	float M02;
	float M10;
	float M11;
	float M12;
	float M20;
	float M21;
	float M22;
	float3x3(float value)
	{
		M00 = value;
		M01 = value;
		M02 = value;
		M10 = value;
		M11 = value;
		M12 = value;
		M20 = value;
		M21 = value;
		M22 = value;
	}
	float3x3() :float3x3(0) {}
	float3x3(float M00, float M01, float M02, float M10, float M11, float M12, float M20, float M21, float M22)
	{
		this->M00 = M00;
		this->M01 = M01;
		this->M02 = M02;
		this->M10 = M10;
		this->M11 = M11;
		this->M12 = M12;
		this->M20 = M20;
		this->M21 = M21;
		this->M22 = M22;
	}

	float3x3(const float4x4 &m) :float3x3(m.M00, m.M01, m.M02, m.M10, m.M11, m.M12, m.M20, m.M21, m.M22) {
	}

	float3x3 getInverse() const
	{
		/// 00 01 02
		/// 10 11 12
		/// 20 21 22
		float Min00 = M11 * M22 - M12 * M21;
		float Min01 = M10 * M22 - M12 * M20;
		float Min02 = M10 * M21 - M11 * M20;

		float det = Min00 * M00 - Min01 * M01 + Min02 * M02;

		if (det == 0)
			return float3x3(0);

		float Min10 = M01 * M22 - M02 * M21;
		float Min11 = M00 * M22 - M02 * M20;
		float Min12 = M00 * M21 - M01 * M20;

		float Min20 = M01 * M12 - M02 * M11;
		float Min21 = M00 * M12 - M02 * M10;
		float Min22 = M00 * M11 - M01 * M10;

		return float3x3(
			(+Min00 / det), (-Min10 / det), (+Min20 / det),
			(-Min01 / det), (+Min11 / det), (-Min21 / det),
			(+Min02 / det), (-Min12 / det), (+Min22 / det));
	}

	float getDeterminant() const
	{
		float Min00 = M11 * M22 - M12 * M21;
		float Min01 = M10 * M22 - M12 * M20;
		float Min02 = M10 * M21 - M11 * M20;

		return Min00 * M00 - Min01 * M01 + Min02 * M02;
	}

	bool IsSingular() const { return getDeterminant() == 0; }
} float3x3;

typedef struct float3x4
{
	float M00;
	float M01;
	float M02;
	float M03;
	float M10;
	float M11;
	float M12;
	float M13;
	float M20;
	float M21;
	float M22;
	float M23;
	float3x4(float value)
	{
		M00 = value;
		M01 = value;
		M02 = value;
		M03 = value;
		M10 = value;
		M11 = value;
		M12 = value;
		M13 = value;
		M20 = value;
		M21 = value;
		M22 = value;
		M23 = value;
	}
	float3x4() :float3x4(0) {}
	float3x4(float M00, float M01, float M02, float M03, float M10, float M11, float M12, float M13, float M20, float M21, float M22, float M23)
	{
		this->M00 = M00;
		this->M01 = M01;
		this->M02 = M02;
		this->M03 = M03;
		this->M10 = M10;
		this->M11 = M11;
		this->M12 = M12;
		this->M13 = M13;
		this->M20 = M20;
		this->M21 = M21;
		this->M22 = M22;
		this->M23 = M23;
	}

	float3x4(const float4x4 &m) :float3x4(m.M00, m.M01, m.M02, m.M03, m.M10, m.M11, m.M12, m.M13, m.M20, m.M21, m.M22, m.M23) {
	}
} float3x4;

float4 operator + (const float4 &v1, const float4 & v2)
{
	return float4{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}
float4 operator - (const float4 & v1, const float4 & v2)
{
	return float4{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}
float4 operator * (const float4 & v1, const float4 & v2)
{
	return float4{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
}
float4 operator / (const float4 & v1, const float4 & v2)
{
	return float4{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w };
}

float3 operator + (const float3 &v1, const float3 & v2)
{
	return float3{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}
float3 operator - (const float3 & v1, const float3 & v2)
{
	return float3{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}
float3 operator * (const float3 & v1, const float3 & v2)
{
	return float3{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
}
float3 operator / (const float3 & v1, const float3 & v2)
{
	return float3{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}

float2 operator + (const float2 &v1, const float2 & v2)
{
	return float2{ v1.x + v2.x, v1.y + v2.y };
}
float2 operator - (const float2 & v1, const float2 & v2)
{
	return float2{ v1.x - v2.x, v1.y - v2.y };
}
float2 operator * (const float2 & v1, const float2 & v2)
{
	return float2{ v1.x * v2.x, v1.y * v2.y };
}
float2 operator / (const float2 & v1, const float2 & v2)
{
	return float2{ v1.x / v2.x, v1.y / v2.y };
}

float4x4 operator + (const float4x4 &m1, const float4x4 &m2)
{
	return float4x4{
		m1.M00 + m2.M00,m1.M01 + m2.M01,m1.M02 + m2.M02,m1.M03 + m2.M03,
		m1.M10 + m2.M10,m1.M11 + m2.M11,m1.M12 + m2.M12,m1.M13 + m2.M13,
		m1.M20 + m2.M20,m1.M21 + m2.M21,m1.M22 + m2.M22,m1.M23 + m2.M23,
		m1.M30 + m2.M30,m1.M31 + m2.M31,m1.M32 + m2.M32,m1.M33 + m2.M33
	};
}
float4x4 operator - (const float4x4 &m1, const float4x4 &m2)
{
	return float4x4{
		m1.M00 - m2.M00,m1.M01 - m2.M01,m1.M02 - m2.M02,m1.M03 - m2.M03,
		m1.M10 - m2.M10,m1.M11 - m2.M11,m1.M12 - m2.M12,m1.M13 - m2.M13,
		m1.M20 - m2.M20,m1.M21 - m2.M21,m1.M22 - m2.M22,m1.M23 - m2.M23,
		m1.M30 - m2.M30,m1.M31 - m2.M31,m1.M32 - m2.M32,m1.M33 - m2.M33
	};
}
float4x4 operator * (const float4x4 &m1, const float4x4 &m2)
{
	return float4x4{
		m1.M00 * m2.M00,m1.M01 * m2.M01,m1.M02 * m2.M02,m1.M03 * m2.M03,
		m1.M10 * m2.M10,m1.M11 * m2.M11,m1.M12 * m2.M12,m1.M13 * m2.M13,
		m1.M20 * m2.M20,m1.M21 * m2.M21,m1.M22 * m2.M22,m1.M23 * m2.M23,
		m1.M30 * m2.M30,m1.M31 * m2.M31,m1.M32 * m2.M32,m1.M33 * m2.M33
	};
}
float4x4 operator / (const float4x4 &m1, const float4x4 &m2)
{
	return float4x4{
		m1.M00 / m2.M00,m1.M01 / m2.M01,m1.M02 / m2.M02,m1.M03 / m2.M03,
		m1.M10 / m2.M10,m1.M11 / m2.M11,m1.M12 / m2.M12,m1.M13 / m2.M13,
		m1.M20 / m2.M20,m1.M21 / m2.M21,m1.M22 / m2.M22,m1.M23 / m2.M23,
		m1.M30 / m2.M30,m1.M31 / m2.M31,m1.M32 / m2.M32,m1.M33 / m2.M33
	};
}

float3x3 operator + (const float3x3 &m1, const float3x3 & m2)
{
	return float3x3{
		m1.M00 + m2.M00,m1.M01 + m2.M01,m1.M02 + m2.M02,
		m1.M10 + m2.M10,m1.M11 + m2.M11,m1.M12 + m2.M12,
		m1.M20 + m2.M20,m1.M21 + m2.M21,m1.M22 + m2.M22
	};
}
float3x3 operator - (const float3x3 & m1, const float3x3 & m2)
{
	return float3x3{
		m1.M00 - m2.M00,m1.M01 - m2.M01,m1.M02 - m2.M02,
		m1.M10 - m2.M10,m1.M11 - m2.M11,m1.M12 - m2.M12,
		m1.M20 - m2.M20,m1.M21 - m2.M21,m1.M22 - m2.M22
	};
}
float3x3 operator * (const float3x3 & m1, const float3x3 & m2)
{
	return float3x3{
		m1.M00 * m2.M00,m1.M01 * m2.M01,m1.M02 * m2.M02,
		m1.M10 * m2.M10,m1.M11 * m2.M11,m1.M12 * m2.M12,
		m1.M20 * m2.M20,m1.M21 * m2.M21,m1.M22 * m2.M22
	};
}
float3x3 operator / (const float3x3 & m1, const float3x3 & m2)
{
	return float3x3{
		m1.M00 / m2.M00,m1.M01 / m2.M01,m1.M02 / m2.M02,
		m1.M10 / m2.M10,m1.M11 / m2.M11,m1.M12 / m2.M12,
		m1.M20 / m2.M20,m1.M21 / m2.M21,m1.M22 / m2.M22
	};
}

float2x2 operator + (const float2x2 &m1, const float2x2 & m2)
{
	return float2x2{
		m1.M00 + m2.M00,m1.M01 + m2.M01,
		m1.M10 + m2.M10,m1.M11 + m2.M11
	};
}
float2x2 operator - (const float2x2 & m1, const float2x2 & m2)
{
	return float2x2{
		m1.M00 - m2.M00,m1.M01 - m2.M01,
		m1.M10 - m2.M10,m1.M11 - m2.M11
	};
}
float2x2 operator * (const float2x2 & m1, const float2x2 & m2)
{
	return float2x2{
		m1.M00 * m2.M00,m1.M01 * m2.M01,
		m1.M10 * m2.M10,m1.M11 * m2.M11
	};
}
float2x2 operator / (const float2x2 & m1, const float2x2 & m2)
{
	return float2x2{
		m1.M00 / m2.M00,m1.M01 / m2.M01,
		m1.M10 / m2.M10,m1.M11 / m2.M11
	};
}

bool any(const float3 &v) {
	return v.x != 0 || v.y != 0 || v.z != 0;
}
bool any(const float4 &v) {
	return v.x != 0 || v.y != 0 || v.z != 0 || v.w != 0;
}

float dot(const float4 &v1, const float4 &v2)
{
	return v1.x *v2.x + v1.y* v2.y + v1.z *v2.z + v1.w*v2.w;
}
float dot(const float3 &v1, const float3 &v2)
{
	return v1.x *v2.x + v1.y* v2.y + v1.z *v2.z;
}
float dot(const float2 &v1, const float2 &v2)
{
	return v1.x *v2.x + v1.y* v2.y;
}

float3 cross(const float3 &v1, const float3 &v2)
{
	return float3(
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x);
}

float2x2 mul(const float2x2 &m1, const float2x2 &m2) {
	return float2x2(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)));
}
float3x3 mul(const float3x3 &m1, const float3x3 &m2)
{
	return float3x3(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)) + (m1.M02 * (m2.M20)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)) + (m1.M02 * (m2.M21)), m1.M00 * (m2.M02) + (m1.M01 * (m2.M12)) + (m1.M02 * (m2.M22)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)) + (m1.M12 * (m2.M20)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)) + (m1.M12 * (m2.M21)), m1.M10 * (m2.M02) + (m1.M11 * (m2.M12)) + (m1.M12 * (m2.M22)), m1.M20 * (m2.M00) + (m1.M21 * (m2.M10)) + (m1.M22 * (m2.M20)), m1.M20 * (m2.M01) + (m1.M21 * (m2.M11)) + (m1.M22 * (m2.M21)), m1.M20 * (m2.M02) + (m1.M21 * (m2.M12)) + (m1.M22 * (m2.M22)));
}
float4x4 mul(const float4x4 &m1, const float4x4 &m2)
{
	return float4x4(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)) + (m1.M02 * (m2.M20)) + (m1.M03 * (m2.M30)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)) + (m1.M02 * (m2.M21)) + (m1.M03 * (m2.M31)), m1.M00 * (m2.M02) + (m1.M01 * (m2.M12)) + (m1.M02 * (m2.M22)) + (m1.M03 * (m2.M32)), m1.M00 * (m2.M03) + (m1.M01 * (m2.M13)) + (m1.M02 * (m2.M23)) + (m1.M03 * (m2.M33)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)) + (m1.M12 * (m2.M20)) + (m1.M13 * (m2.M30)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)) + (m1.M12 * (m2.M21)) + (m1.M13 * (m2.M31)), m1.M10 * (m2.M02) + (m1.M11 * (m2.M12)) + (m1.M12 * (m2.M22)) + (m1.M13 * (m2.M32)), m1.M10 * (m2.M03) + (m1.M11 * (m2.M13)) + (m1.M12 * (m2.M23)) + (m1.M13 * (m2.M33)), m1.M20 * (m2.M00) + (m1.M21 * (m2.M10)) + (m1.M22 * (m2.M20)) + (m1.M23 * (m2.M30)), m1.M20 * (m2.M01) + (m1.M21 * (m2.M11)) + (m1.M22 * (m2.M21)) + (m1.M23 * (m2.M31)), m1.M20 * (m2.M02) + (m1.M21 * (m2.M12)) + (m1.M22 * (m2.M22)) + (m1.M23 * (m2.M32)), m1.M20 * (m2.M03) + (m1.M21 * (m2.M13)) + (m1.M22 * (m2.M23)) + (m1.M23 * (m2.M33)), m1.M30 * (m2.M00) + (m1.M31 * (m2.M10)) + (m1.M32 * (m2.M20)) + (m1.M33 * (m2.M30)), m1.M30 * (m2.M01) + (m1.M31 * (m2.M11)) + (m1.M32 * (m2.M21)) + (m1.M33 * (m2.M31)), m1.M30 * (m2.M02) + (m1.M31 * (m2.M12)) + (m1.M32 * (m2.M22)) + (m1.M33 * (m2.M32)), m1.M30 * (m2.M03) + (m1.M31 * (m2.M13)) + (m1.M32 * (m2.M23)) + (m1.M33 * (m2.M33)));
}
float2 mul(const float2 &v, const float2x2 &m) {
	return float2(v.x * m.M00 + v.y*m.M10, v.x*m.M01 + v.y*m.M11);
}
float3 mul(const float3 &v, const float3x3 &m)
{
	return float3(v.x * (m.M00) + (v.y * (m.M10)) + (v.z * (m.M20)), v.x * (m.M01) + (v.y * (m.M11)) + (v.z * (m.M21)), v.x * (m.M02) + (v.y * (m.M12)) + (v.z * (m.M22)));
}
float4 mul(const float4 &v, const float4x4 &m)
{
	return float4(v.x * (m.M00) + (v.y * (m.M10)) + (v.z * (m.M20)) + (v.w * (m.M30)), v.x * (m.M01) + (v.y * (m.M11)) + (v.z * (m.M21)) + (v.w * (m.M31)), v.x * (m.M02) + (v.y * (m.M12)) + (v.z * (m.M22)) + (v.w * (m.M32)), v.x * (m.M03) + (v.y * (m.M13)) + (v.z * (m.M23)) + (v.w * (m.M33)));
}

float3 minf(float3 x, float3 y) {
	return float3(min(x.x, y.x), min(x.y, y.y), min(x.z, y.z));
}

float3 maxf(float3 x, float3 y) {
	return float3(max(x.x, y.x), max(x.y, y.y), max(x.z, y.z));
}

float2x2 transpose(const float2x2 &m)
{
	return float2x2(m.M00, m.M10, m.M01, m.M11);
}
float3x3 transpose(const float3x3 &m)
{
	return float3x3(m.M00, m.M10, m.M20, m.M01, m.M11, m.M21, m.M02, m.M12, m.M22);
}
float4x4 transpose(const float4x4 &m)
{
	return float4x4(m.M00, m.M10, m.M20, m.M30, m.M01, m.M11, m.M21, m.M31, m.M02, m.M12, m.M22, m.M32, m.M03, m.M13, m.M23, m.M33);
}

float length(const float2 &v)
{
	return sqrtf(v.x*v.x + v.y*v.y);
}
float length(const float3 &v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
float length(const float4 &v)
{
	return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

float2 normalize(const float2 &v)
{
	float l = length(v);
	return l == 0 ? v : v * (1 / l);
}
float3 normalize(const float3 &v)
{
	float l = length(v);
	return l == 0 ? v : v * (1 / l);
}
float4 normalize(const float4 &v)
{
	float l = length(v);
	return l == 0 ? v : v * (1 / l);
}


/// matrices
/// <summary>
/// Builds a mat using specified offsets.
/// </summary>
/// <param name="xslide">x offsets</param>
/// <param name="yslide">y offsets</param>
/// <param name="zslide">z offsets</param>
/// <returns>A mat structure that contains a translated transformation </returns>
float4x4 Translate(float xoffset, float yoffset, float zoffset)
{
	return float4x4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		xoffset, yoffset, zoffset, 1
	);
}
/// <summary>
/// Builds a mat using specified offsets.
/// </summary>
/// <param name="vec">A Vector structure that contains the x-coordinate, y-coordinate, and z-coordinate offsets.</param>
/// <returns>A mat structure that contains a translated transformation </returns>
float4x4 Translate(float3 vec)
{
	return Translate(vec.x, vec.y, vec.z);
}
//

// Rotations
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in radian for rotation</param>
float4x4 RotateZ(float alpha)
{
	float cos = cosf(alpha);
	float sin = sinf(alpha);
	return float4x4(
		cos, -sin, 0, 0,
		sin, cos, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in grades for rotation</param>
float4x4 RotateZGrad(float alpha)
{
	return RotateZ(alpha * PI / 180);
}
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in radian for rotation</param>
float4x4 RotateY(float alpha)
{
	float cos = cosf(alpha);
	float sin = sinf(alpha);
	return float4x4(
		cos, 0, -sin, 0,
		0, 1, 0, 0,
		sin, 0, cos, 0,
		0, 0, 0, 1
	);
}
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in grades for rotation</param>
float4x4 RotateYGrad(float alpha)
{
	return RotateY(alpha * PI / 180);
}
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in radian for rotation</param>
float4x4 RotateX(float alpha)
{
	float cos = cosf(alpha);
	float sin = sinf(alpha);
	return float4x4(
		1, 0, 0, 0,
		0, cos, -sin, 0,
		0, sin, cos, 0,
		0, 0, 0, 1
	);
}
/// <summary>
/// Rotation mat around Z axis
/// </summary>
/// <param name="alpha">value in grades for rotation</param>
float4x4 RotateXGrad(float alpha)
{
	return RotateX(alpha * PI / 180);
}
float4x4 Rotate(float angle, const float3 & dir)
{
	float x = dir.x;
	float y = dir.y;
	float z = dir.z;
	float cos = cosf(angle);
	float sin = sinf(angle);

	return float4x4(
		x * x * (1 - cos) + cos, y * x * (1 - cos) + z * sin, z * x * (1 - cos) - y * sin, 0,
		x * y * (1 - cos) - z * sin, y * y * (1 - cos) + cos, z * y * (1 - cos) + x * sin, 0,
		x * z * (1 - cos) + y * sin, y * z * (1 - cos) - x * sin, z * z * (1 - cos) + cos, 0,
		0, 0, 0, 1
	);
}
float4x4 RotateRespectTo(const float3 &center, const float3 &direction, float angle)
{
	return mul(Translate(center), mul(Rotate(angle, direction), Translate(center * -1.0f)));
}
float4x4 RotateGrad(float angle, const float3 & dir)
{
	return Rotate(PI * angle / 180, dir);
}

//

// Scale

float4x4 Scale(float xscale, float yscale, float zscale)
{
	return float4x4(
		xscale, 0, 0, 0,
		0, yscale, 0, 0,
		0, 0, zscale, 0,
		0, 0, 0, 1);
}
float4x4 Scale(const float3 & size)
{
	return Scale(size.x, size.y, size.z);
}

float4x4 ScaleRespectTo(const float3 & center, const float3 & size)
{
	return mul(mul(Translate(center), Scale(size)), Translate(center * -1));
}
float4x4 ScaleRespectTo(const float3 & center, float sx, float sy, float sz)
{
	return ScaleRespectTo(center, float3(sx, sy, sz));
}

//

// Viewing

float4x4 LookAtLH(const float3 & camera, const float3 & target, const float3 & upVector)
{
	float3 zaxis = normalize(target - camera);
	float3 xaxis = normalize(cross(upVector, zaxis));
	float3 yaxis = cross(zaxis, xaxis);

	return float4x4(
		xaxis.x, yaxis.x, zaxis.x, 0,
		xaxis.y, yaxis.y, zaxis.y, 0,
		xaxis.z, yaxis.z, zaxis.z, 0,
		-dot(xaxis, camera), -dot(yaxis, camera), -dot(zaxis, camera), 1);
}

float4x4 LookAtRH(const float3 & camera, const float3 & target, const float3 & upVector)
{
	float3 zaxis = normalize(camera - target);
	float3 xaxis = normalize(cross(upVector, zaxis));
	float3 yaxis = cross(zaxis, xaxis);

	return float4x4(
		xaxis.x, yaxis.x, zaxis.x, 0,
		xaxis.y, yaxis.y, zaxis.y, 0,
		xaxis.z, yaxis.z, zaxis.z, 0,
		-dot(xaxis, camera), -dot(yaxis, camera), -dot(zaxis, camera), 1);
}

//

// Projection Methods

/// <summary>
/// Returns the near plane distance to a given projection
/// </summary>
/// <param name="proj">A mat structure containing the projection</param>
/// <returns>A float value representing the distance.</returns>
float ZnearPlane(const float4x4 &proj)
{
	float4 pos = mul(float4(0, 0, 0, 1), proj.getInverse());
	return pos.z / pos.w;
}

/// <summary>
/// Returns the far plane distance to a given projection
/// </summary>
/// <param name="proj">A mat structure containing the projection</param>
/// <returns>A float value representing the distance.</returns>
float ZfarPlane(const float4x4 &proj)
{
	float4 targetPos = mul(float4(0, 0, 1, 1), proj.getInverse());
	return targetPos.z / targetPos.w;
}

float4x4 PerspectiveFovLH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
{
	float h = 1.0f / tanf(fieldOfView / 2);
	float w = h * aspectRatio;

	return float4x4(
		w, 0, 0, 0,
		0, h, 0, 0,
		0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
		0, 0, -znearPlane * zfarPlane / (zfarPlane - znearPlane), 0);
}

float4x4 PerspectiveFovRH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
{
	float h = 1.0f / tanf(fieldOfView / 2);
	float w = h * aspectRatio;

	return float4x4(
		w, 0, 0, 0,
		0, h, 0, 0,
		0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
		0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
}

float4x4 PerspectiveLH(float width, float height, float znearPlane, float zfarPlane)
{
	return float4x4(
		2 * znearPlane / width, 0, 0, 0,
		0, 2 * znearPlane / height, 0, 0,
		0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
		0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
}

float4x4 PerspectiveRH(float width, float height, float znearPlane, float zfarPlane)
{
	return float4x4(
		2 * znearPlane / width, 0, 0, 0,
		0, 2 * znearPlane / height, 0, 0,
		0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
		0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
}

float4x4 OrthoLH(float width, float height, float znearPlane, float zfarPlane)
{
	return float4x4(
		2 / width, 0, 0, 0,
		0, 2 / height, 0, 0,
		0, 0, 1 / (zfarPlane - znearPlane), 0,
		0, 0, znearPlane / (znearPlane - zfarPlane), 1);
}

float4x4 OrthoRH(float width, float height, float znearPlane, float zfarPlane)
{
	return float4x4(
		2 / width, 0, 0, 0,
		0, 2 / height, 0, 0,
		0, 0, 1 / (znearPlane - zfarPlane), 0,
		0, 0, znearPlane / (znearPlane - zfarPlane), 1);
}

//

#endif // !GMATH_H
