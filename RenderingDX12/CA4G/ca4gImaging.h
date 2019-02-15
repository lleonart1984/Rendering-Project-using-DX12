#pragma once

#include "DirectXTex/DirectXTex.h"

struct TextureData {
	int const Width;
	int const Height;
	int const MipMaps;
	union {
		int const ArraySlices;
		int const Depth;
	};
	DXGI_FORMAT const Format;
	byte* const Data;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints;

	long long const DataSize;
	bool const FlipY;
	D3D12_RESOURCE_DIMENSION Dimension;

	TextureData(int width, int height, int mipMaps, int slices, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false)
		:Width(width), Height(height),
		MipMaps(mipMaps),
		ArraySlices(slices),
		Format(format),
		Data(data),
		DataSize(dataSize),
		FlipY(flipY),
		pixelStride(DirectX::BitsPerPixel(format)),
		Dimension(D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		footprints = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[mipMaps * slices];
		int index = 0;
		UINT64 offset = 0;
		for (int s = 0; s < slices; s++)
		{
			int w = Width;
			int h = Height;
			for (int m = 0; m < mipMaps; m++)
			{
				footprints[index].Footprint.Width = w;
				footprints[index].Footprint.Height = h;
				footprints[index].Footprint.Format = format;
				footprints[index].Footprint.RowPitch = pixelStride * w;
				footprints[index].Footprint.Depth = 1;
				footprints[index].Offset = offset;

				offset += w * h * pixelStride;

				w /= 2; w = max(1, w);
				h /= 2; h = max(1, h);

				index++;
			}
		}
	}

	TextureData(int width, int height, int depth, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false)
		:Width(width), Height(height),
		MipMaps(1),
		Depth(depth),
		Format(format),
		Data(data),
		DataSize(dataSize),
		FlipY(flipY),
		pixelStride(DirectX::BitsPerPixel(format)),
		Dimension(D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		footprints = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[1];

		footprints[0].Footprint.Width = width;
		footprints[0].Footprint.Height = height;
		footprints[0].Footprint.Depth = depth;
		footprints[0].Footprint.Format = format;
		footprints[0].Footprint.RowPitch = pixelStride * width;
		footprints[0].Offset = 0;
	}

	~TextureData() {
		delete[] Data;
		delete[] footprints;
	}

	static TextureData* CreateEmpty(int width, int height, int mipMaps, int slices, DXGI_FORMAT format) {
		
		int pixelStride = DirectX::BitsPerPixel(format);
		UINT64 total = 0;
		int w = width, h = height;
		for (int i = 0; i < mipMaps; i++)
		{
			total += w * h*pixelStride;
			w /= 2;
			h /= 2;
			w = max(1, w);
			h = max(1, h);
		}
		total *= slices;
		byte* data = new byte[total];
		ZeroMemory(data, total);
		return new TextureData(width, height, mipMaps, slices, format, data, total, false);
	}

	static TextureData* CreateEmpty(int width, int height, int depth, DXGI_FORMAT format) {
		int pixelStride = DirectX::BitsPerPixel(format);
		UINT64 total = width * height* depth * pixelStride;
		byte* data = new byte[total];
		ZeroMemory(data, total);
		return new TextureData(width, height, depth, format, data, total, false);
	}

	static TextureData* LoadFromFile(const char * filename) {

		static bool InitializedWic = false;

		if (!InitializedWic) {
			void* res = nullptr;
			CoInitialize(res);
			InitializedWic = true;
		}
		
		wchar_t* file = new wchar_t[strlen(filename) + 1];
		OemToCharW(filename, file);
		DirectX::TexMetadata metadata;
		DirectX::ScratchImage scratchImage;

		bool notDDS;
		bool notWIC;
		bool notTGA;

		if (
			(notDDS=FAILED(DirectX::LoadFromDDSFile(file, 0, &metadata, scratchImage)))
		&&	(notWIC=FAILED(DirectX::LoadFromWICFile(file, 0, &metadata, scratchImage)))
		&&	(notTGA=FAILED(DirectX::LoadFromTGAFile(file, &metadata, scratchImage))))
			return nullptr;

		return CreateTextureDataFromScratchImage(metadata, scratchImage, true);
	}

	//using DECODE_FUNCTION = float(byte*);
	//using ENCODE_FUNCTION = void (float, byte*);

	//static float DecodeByteAsUnsigned(byte* ptr)
	//{
	//	return (*ptr);
	//}
	//static float DecodeByteAsUnsignedNorm(byte* ptr)
	//{
	//	return (*ptr) / 255.0;
	//}
	//static float DecodeByteAsSigned(byte* ptr)
	//{
	//	return (char)(*ptr);
	//}
	//static float DecodeByteAsSignedNorm(byte* ptr)
	//{
	//	return 2 * ((*ptr) / 255.0) - 1;
	//}

	//static void EncodeByteAsUnsigned(float value, byte* ptr)
	//{
	//	(*ptr) = (byte)value;
	//}
	//static void EncodeByteAsUnsignedNorm(float value, byte* ptr)
	//{
	//	(*ptr) = (byte)(value * 255.0);
	//}
	//static void EncodeByteAsSigned(float value, byte* ptr)
	//{
	//	(*ptr) = (byte)(char)value;
	//}
	//static void EncodeByteAsSignedNorm(float value, byte* ptr)
	//{
	//	(*ptr) = (value*0.5 + 0.5) * 255;
	//}

	//static void GetFormatLayout(DXGI_FORMAT format,
	//	int &rOffset, int &gOffset, int &bOffset, int &aOffset,
	//	int &bytesPerComponent,
	//	DECODE_FUNCTION* &decoding,
	//	ENCODE_FUNCTION* &encoding) {

	//	// Components offsets
	//	switch (format) {
	//	case DXGI_FORMAT_R8G8B8A8_SINT:
	//	case DXGI_FORMAT_R8G8B8A8_SNORM:
	//	case DXGI_FORMAT_R8G8B8A8_UINT:
	//	case DXGI_FORMAT_R8G8B8A8_UNORM:
	//	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	//	{
	//		rOffset = 0; gOffset = 1; bOffset = 2; aOffset = 3;
	//		break;
	//	}
	//	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM:
	//	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM:
	//	{
	//		rOffset = 2; gOffset = 1; bOffset = 0; aOffset = 3;
	//		break;
	//	}
	//	case DXGI_FORMAT_A8_UNORM:
	//	case DXGI_FORMAT_R8_UNORM:
	//	case DXGI_FORMAT_P8:
	//	{
	//		rOffset = gOffset = bOffset = aOffset = 0; // all components equals to value mapped
	//		break;
	//	}

	//	default:
	//		throw AfterShow(CA4G_Errors_UnsupportedFormat);
	//	}

	//	// Per Component bytes length
	//	switch (format) {
	//	case DXGI_FORMAT_R8G8B8A8_SINT:
	//	case DXGI_FORMAT_R8G8B8A8_SNORM:
	//	case DXGI_FORMAT_R8G8B8A8_UINT:
	//	case DXGI_FORMAT_R8G8B8A8_UNORM:
	//	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM:
	//	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM:
	//	case DXGI_FORMAT_P8:
	//	case DXGI_FORMAT_A8_UNORM:
	//	case DXGI_FORMAT_R8_UNORM:
	//	{
	//		bytesPerComponent = 1;
	//		break;
	//	}
	//	default:
	//		throw "Not supported";
	//	}

	//	// Function to decode
	//	switch (format) {
	//	case DXGI_FORMAT_R8G8B8A8_SNORM:
	//	{
	//		decoding = &DecodeByteAsSignedNorm;
	//		encoding = &EncodeByteAsSignedNorm;
	//		break;
	//	}
	//	case DXGI_FORMAT_R8G8B8A8_SINT:
	//	{
	//		decoding = &DecodeByteAsSigned;
	//		encoding = &EncodeByteAsSigned;
	//		break;
	//	}
	//	case DXGI_FORMAT_R8G8B8A8_UINT:
	//	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	//	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	//	{
	//		decoding = &DecodeByteAsUnsigned;
	//		encoding = &EncodeByteAsUnsigned;
	//		break;
	//	}
	//	case DXGI_FORMAT_R8G8B8A8_UNORM:
	//	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8A8_UNORM:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	//	case DXGI_FORMAT_B8G8R8X8_UNORM:
	//	case DXGI_FORMAT_P8:
	//	case DXGI_FORMAT_R8_UNORM:
	//	case DXGI_FORMAT_A8_UNORM:
	//	{
	//		decoding = &DecodeByteAsUnsignedNorm;
	//		encoding = &EncodeByteAsUnsignedNorm;
	//		break;
	//	}
	//	default:
	//		throw "Not supported";
	//	}
	//}

	//float4 DecodePixel(int x, int y, int mip = 0, int arr = 0) {
	//	D3D12_PLACED_SUBRESOURCE_FOOTPRINT &f = footprints[SubresourceIndex(mip, arr)];

	//	byte* pixelPtr = Data
	//		+ f.Offset // Positioned at subresource
	//		+ y * f.Footprint.RowPitch // Positioned at row start
	//		+ x * pixelStride; // Positioned at pixel
	//	
	//	float r = decoding(pixelPtr + rOffset * bytesPerComponent);
	//	float g = decoding(pixelPtr + gOffset * bytesPerComponent);
	//	float b = decoding(pixelPtr + bOffset * bytesPerComponent);
	//	float a = decoding(pixelPtr + aOffset * bytesPerComponent);
	//	return float4(r, g, b, a);
	//}

	//float4 SafeDecodePixel(int x, int y, int mip = 0, int arr = 0) {
	//	D3D12_PLACED_SUBRESOURCE_FOOTPRINT &f = footprints[SubresourceIndex(mip, arr)];

	//	if (x < 0) x = 0;
	//	if (y < 0) y = 0;
	//	if (x >= f.Footprint.Width) x = f.Footprint.Width - 1;
	//	if (y >= f.Footprint.Height) y = f.Footprint.Height - 1;

	//	byte* pixelPtr = Data
	//		+ f.Offset // Positioned at subresource
	//		+ y * f.Footprint.RowPitch // Positioned at row start
	//		+ x * pixelStride; // Positioned at pixel

	//	float r = decoding(pixelPtr + rOffset * bytesPerComponent);
	//	float g = decoding(pixelPtr + gOffset * bytesPerComponent);
	//	float b = decoding(pixelPtr + bOffset * bytesPerComponent);
	//	float a = decoding(pixelPtr + aOffset * bytesPerComponent);
	//	return float4(r, g, b, a);
	//}

	//void EncodePixel(float4 value, int x, int y, int mip = 0, int arr = 0) {
	//	D3D12_PLACED_SUBRESOURCE_FOOTPRINT &f = footprints[SubresourceIndex(mip, arr)];

	//	byte* pixelPtr = Data
	//		+ f.Offset // Positioned at subresource
	//		+ y * f.Footprint.RowPitch // Positioned at row start
	//		+ x * pixelStride; // Positioned at pixel

	//	encoding(value.x, pixelPtr + rOffset * bytesPerComponent);
	//	if (gOffset != rOffset) encoding(value.y, pixelPtr + gOffset * bytesPerComponent);
	//	if (bOffset != rOffset) encoding(value.z, pixelPtr + bOffset * bytesPerComponent);
	//	if (aOffset != rOffset) encoding(value.w, pixelPtr + aOffset * bytesPerComponent);
	//}

	/*static int GetPixelStride(DXGI_FORMAT format) {
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM:

		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return 4;

		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_TYPELESS:

		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_P8:
			return 1;
		default:
			return 1;
		}
	}*/

private:
	int pixelStride;

	static TextureData* CreateTextureDataFromScratchImage(DirectX::TexMetadata &metadata, DirectX::ScratchImage &image, bool flipY) {
		byte* buffer = new byte[image.GetPixelsSize()];
		memcpy(buffer, image.GetPixels(), image.GetPixelsSize());

		return metadata.dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ?
			new TextureData(metadata.width, metadata.height, metadata.mipLevels, metadata.arraySize, metadata.format, buffer, image.GetPixelsSize(), flipY)
			: new TextureData(metadata.width, metadata.height, metadata.depth, metadata.format, buffer, image.GetPixelsSize(), flipY);
	}

	
};