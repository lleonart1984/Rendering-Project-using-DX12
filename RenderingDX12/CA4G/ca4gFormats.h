#pragma once

#pragma region Formats
template<typename T>
struct Formats {
	static const DXGI_FORMAT Value = DXGI_FORMAT_UNKNOWN;
	static const int Size = 1;
};
template<>
struct Formats<char> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R8_SINT;
	static const int Size = 1;
};
template<>
struct Formats<float> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32_FLOAT;
	static const int Size = 4;
};
template<>
struct Formats <int> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32_SINT;
	static const int Size = 4;
};

template<>
struct Formats <ARGB> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_B8G8R8A8_UNORM;
	static const int Size = 4;
};

template<>
struct Formats <RGBA> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const int Size = 4;
};

template<>
struct Formats <unsigned int> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32_UINT;
	static const int Size = 4;
};
template<>
struct Formats <float2> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_FLOAT;
	static const int Size = 8;
};
template<>
struct Formats <float3> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_FLOAT;
	static const int Size = 12;
};
template<>
struct Formats <float4> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_FLOAT;
	static const int Size = 16;
};

template<>
struct Formats <int2> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_SINT;
	static const int Size = 8;
};
template<>
struct Formats <int3> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_SINT;
	static const int Size = 12;
};
template<>
struct Formats <int4> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_SINT;
	static const int Size = 16;
};

template<>
struct Formats <uint2> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_UINT;
	static const int Size = 8;
};
template<>
struct Formats <uint3> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_UINT;
	static const int Size = 12;
};
template<>
struct Formats <uint4> {
	static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_UINT;
	static const int Size = 16;
};
#pragma endregion