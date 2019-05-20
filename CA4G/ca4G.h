#ifndef CA4G_H
#define CA4G_H

#include <Windows.h>
#include <atlbase.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#pragma region D3D12FallbackDevice.h

struct EMULATED_GPU_POINTER
{
	UINT32 OffsetInBytes;
	UINT32 DescriptorHeapIndex;
};

struct WRAPPED_GPU_POINTER
{
	union
	{
		EMULATED_GPU_POINTER EmulatedGpuPtr;
		D3D12_GPU_VIRTUAL_ADDRESS GpuVA;
	};

	WRAPPED_GPU_POINTER operator+(UINT64 offset)
	{
		WRAPPED_GPU_POINTER pointer = *this;
		pointer.GpuVA += offset;
		return pointer;
	}
};

typedef struct D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC
{
	FLOAT Transform[3][4];
	UINT InstanceID : 24;
	UINT InstanceMask : 8;
	UINT InstanceContributionToHitGroupIndex : 24;
	UINT Flags : 8;
	WRAPPED_GPU_POINTER AccelerationStructure;
}     D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC;

class
	_declspec(uuid("539e5c40-df25-4c7d-81d8-6537f54306ed"))
	ID3D12RaytracingFallbackStateObject : public IUnknown
{
public:
	virtual ~ID3D12RaytracingFallbackStateObject() {};

	virtual void *STDMETHODCALLTYPE GetShaderIdentifier(
		_In_  LPCWSTR pExportName) = 0;

	virtual UINT64 STDMETHODCALLTYPE GetShaderStackSize(
		_In_  LPCWSTR pExportName) = 0;

	virtual UINT64 STDMETHODCALLTYPE GetPipelineStackSize(void) = 0;

	virtual void STDMETHODCALLTYPE SetPipelineStackSize(
		UINT64 PipelineStackSizeInBytes) = 0;

	virtual ID3D12StateObject *GetStateObject() = 0;
};

class
	_declspec(uuid("348a2a6b-6760-4b78-a9a7-1758b6f78d46"))
	ID3D12RaytracingFallbackCommandList : public IUnknown
{
public:
	virtual ~ID3D12RaytracingFallbackCommandList() {}

	virtual void BuildRaytracingAccelerationStructure(
		_In_  const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *pDesc,
		_In_  UINT NumPostbuildInfoDescs,
		_In_reads_opt_(NumPostbuildInfoDescs)  const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *pPostbuildInfoDescs) = 0;

	virtual void STDMETHODCALLTYPE EmitRaytracingAccelerationStructurePostbuildInfo(
		_In_  const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *pDesc,
		_In_  UINT NumSourceAccelerationStructures,
		_In_reads_(NumSourceAccelerationStructures)  const D3D12_GPU_VIRTUAL_ADDRESS *pSourceAccelerationStructureData) = 0;

	virtual void STDMETHODCALLTYPE CopyRaytracingAccelerationStructure(
		_In_  D3D12_GPU_VIRTUAL_ADDRESS DestAccelerationStructureData,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS SourceAccelerationStructureData,
		_In_  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE Mode) = 0;

	virtual void STDMETHODCALLTYPE SetDescriptorHeaps(
		_In_  UINT NumDescriptorHeaps,
		_In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps) = 0;

	virtual void STDMETHODCALLTYPE SetTopLevelAccelerationStructure(
		_In_  UINT RootParameterIndex,
		_In_  WRAPPED_GPU_POINTER  BufferLocation) = 0;

	virtual void STDMETHODCALLTYPE SetPipelineState1(
		_In_  ID3D12RaytracingFallbackStateObject *pStateObject) = 0;

	virtual void STDMETHODCALLTYPE DispatchRays(
		_In_  const D3D12_DISPATCH_RAYS_DESC *pDesc) = 0;
};

class
	_declspec(uuid("0a662ea0-ab43-423a-848f-4824ae4b25ba"))
	ID3D12RaytracingFallbackDevice : public IUnknown
{
public:
	virtual ~ID3D12RaytracingFallbackDevice() {};

	virtual bool UsingRaytracingDriver() = 0;

	// Automatically determine how to create WRAPPED_GPU_POINTER based on UsingRaytracingDriver()
	virtual WRAPPED_GPU_POINTER GetWrappedPointerSimple(UINT32 DescriptorHeapIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuVA) = 0;

	// Pre-condition: UsingRaytracingDriver() must be false
	virtual WRAPPED_GPU_POINTER GetWrappedPointerFromDescriptorHeapIndex(UINT32 DescriptorHeapIndex, UINT32 OffsetInBytes = 0) = 0;

	// Pre-condition: UsingRaytracingDriver() must be true
	virtual WRAPPED_GPU_POINTER GetWrappedPointerFromGpuVA(D3D12_GPU_VIRTUAL_ADDRESS gpuVA) = 0;

	virtual D3D12_RESOURCE_STATES GetAccelerationStructureResourceState() = 0;

	virtual UINT STDMETHODCALLTYPE GetShaderIdentifierSize(void) = 0;

	virtual HRESULT STDMETHODCALLTYPE CreateStateObject(
		const D3D12_STATE_OBJECT_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_  void **ppStateObject) = 0;

	virtual void STDMETHODCALLTYPE GetRaytracingAccelerationStructurePrebuildInfo(
		_In_  const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS *pDesc,
		_Out_  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO *pInfo) = 0;

	virtual void QueryRaytracingCommandList(
		ID3D12GraphicsCommandList *pCommandList,
		REFIID riid,
		_COM_Outptr_  void **ppRaytracingCommandList) = 0;

	virtual HRESULT STDMETHODCALLTYPE CreateRootSignature(
		_In_  UINT nodeMask,
		_In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
		_In_  SIZE_T blobLengthInBytes,
		REFIID riid,
		_COM_Outptr_  void **ppvRootSignature) = 0;

	virtual HRESULT D3D12SerializeVersionedRootSignature(
		_In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
		_Out_ ID3DBlob** ppBlob,
		_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob) = 0;

	virtual HRESULT WINAPI D3D12SerializeRootSignature(
		_In_ const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
		_In_ D3D_ROOT_SIGNATURE_VERSION Version,
		_Out_ ID3DBlob** ppBlob,
		_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob) = 0;
};

enum CreateRaytracingFallbackDeviceFlags
{
	None = 0x0,
	ForceComputeFallback = 0x1,
	EnableRootDescriptorsInShaderRecords = 0x2
};

HRESULT D3D12CreateRaytracingFallbackDevice(
	_In_ ID3D12Device *pDevice,
	_In_ DWORD createRaytracingFallbackDeviceFlags,
	_In_ UINT NodeMask,
	_In_ REFIID riid,
	_COM_Outptr_opt_ void** ppDevice);

#pragma endregion

namespace CA4G {
#pragma region Class Definitions
	class Bundle;
	class Clearing;
	class Creating;
	class Loading;
	class Copying;
	class CommandListManager;
	class CopyingManager;
	class ComputeManager;
	class GraphicsManager;
	class DXRManager;
	class Presenter;
	class DeviceManager;
	template<typename ...A> class PipelineBindings;
	class ComputePipelineBindings;
	class GraphicsPipelineBindings;
	class ResourceWrapper;
	class ResourceView;
	class Buffer;
	class Texture1D;
	class Texture2D;
	class CubeTexture;
	class Texture2DMS;
	class Texture3D;
	struct Sampler;
	class GPUScheduler;
	class SceneBuilder;
	class GeometryCollection;
	class TriangleGeometryCollection;
	class ProceduralGeometryCollection;
	class InstanceCollection;
	class HitGroupManager;
	template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type> class DynamicStateBindingOf;
	class DXILManager;
	template<typename C> class DXIL_Library;
	class IRTProgram;
	template<typename L> class RTProgram;
	class RTPipelineManager;
#pragma endregion
}

#pragma region DX COM OBJECTS

typedef CComPtr<ID3D12DescriptorHeap> DX_DescriptorHeap;
typedef CComPtr<ID3D12Device5> DX_Device;
typedef CComPtr<ID3D12RaytracingFallbackDevice> DX_FallbackDevice;
typedef CComPtr<ID3D12RaytracingFallbackCommandList> DX_FallbackCommandList;
typedef CComPtr<ID3D12RaytracingFallbackStateObject> DX_FallbackStateObject;

typedef CComPtr<ID3D12GraphicsCommandList4> DX_CommandList;
typedef CComPtr<ID3D12CommandAllocator> DX_CommandAllocator;
typedef CComPtr<ID3D12CommandQueue> DX_CommandQueue;
typedef CComPtr<ID3D12Fence1> DX_Fence;
typedef CComPtr<ID3D12Heap1> DX_Heap;
typedef CComPtr<ID3D12PipelineState> DX_PipelineState;
typedef CComPtr<ID3D12Resource1> DX_Resource;
typedef CComPtr<ID3D12RootSignature> DX_RootSignature;
typedef CComPtr<ID3D12RootSignatureDeserializer> DX_RootSignatureDeserializer;
typedef CComPtr<ID3D12StateObject> DX_StateObject;
typedef CComPtr<ID3D12StateObjectProperties> DX_StateObjectProperties;

#pragma endregion

#define CA4G_MAX_NUMBER_OF_WORKERS 8
#define CA4G_SUPPORTED_ENGINES 4
#define CA4G_SUPPORTED_BUFFERING 3

#pragma region ca4GErrors.h

#include <comdef.h>
#include <stdexcept>

namespace CA4G {

	// Represents different error code this engine can report.
	enum CA4G_Errors {
		// Some error occurred.
		CA4G_Errors_Any,
		// The Pipeline State Object has a bad construction.
		CA4G_Errors_BadPSOConstruction,
		// The Signature has a bad construction
		CA4G_Errors_BadSignatureConstruction,
		// The image is in an unsupported format.
		CA4G_Errors_UnsupportedFormat,
		// Some shader were not found
		CA4G_Errors_ShaderNotFound,
		// Some initialization failed because memory was over
		CA4G_Errors_RunOutOfMemory,
		// Invalid Operation
		CA4G_Errors_Invalid_Operation,
		// Fallback raytracing device was not supported
		CA4G_Errors_Unsupported_Fallback
	};

	class CA4GException : public std::exception {
	public:
		CA4GException(const char* const message) :std::exception(message) {}

		static CA4GException FromError(CA4G_Errors error, const char* arg = nullptr, HRESULT hr = S_OK);

	};
}

#pragma endregion

#pragma region ca4GMemory.h

namespace CA4G {

	template<typename S>
	class gObj {
		friend S;
		template<typename T> friend class list; // I dont like it... :(
		template<typename T> friend class gObj;

	private:
		S *_this;
		volatile long* counter;

		void AddReference();

		void RemoveReference();

	public:
		gObj() : _this(nullptr), counter(nullptr) {
		}
		gObj(S* self) : _this(self), counter(self ? new long(1) : nullptr) {
		}

		inline bool isNull() const { return _this == nullptr; }

		// Copy constructor
		gObj(const gObj<S>& other) {
			this->counter = other.counter;
			this->_this = other._this;
			if (!isNull())
				AddReference();
		}

		template <typename Subtype>
		gObj(const gObj<Subtype>& other) {
			this->counter = other.counter;
			this->_this = (S*)other._this;
			if (!isNull())
				AddReference();
		}

		gObj<S>& operator = (const gObj<S>& other) {
			if (!isNull())
				RemoveReference();
			this->counter = other.counter;
			this->_this = other._this;
			if (!isNull())
				AddReference();
			return *this;
		}

		bool operator == (const gObj<S> &other) {
			return other._this == _this;
		}

		bool operator != (const gObj<S> &other) {
			return other._this != _this;
		}

		template<typename A>
		auto& operator[](A arg) {
			return (*_this)[arg];
		}

		~gObj() {
			if (!isNull())
				RemoveReference();
		}

		//Dereference operator
		S& operator*()
		{
			return *_this;
		}
		//Member Access operator
		S* operator->()
		{
			return _this;
		}

		template<typename T>
		gObj<T> Dynamic_Cast() {
			gObj<T> obj;
			obj._this = dynamic_cast<T*>(_this);
			obj.counter = counter;
			obj.AddReference();
			return obj;
		}

		template<typename T>
		gObj<T> Static_Cast() {
			gObj<T> obj;
			obj._this = static_cast<T*>(_this);
			obj.counter = counter;
			obj.AddReference();
			return obj;
		}

		operator bool() const {
			return !isNull();
		}
	};
}

#pragma endregion

#pragma region ca4GShaders.h

namespace CA4G {

#define CompiledShader(s) (s),ARRAYSIZE(s) 

	class ShaderLoader {
	public:
		// Use this method to load a bytecode
		static D3D12_SHADER_BYTECODE FromFile(const char* bytecodeFilePath);

		// Use this method to load a bytecode from a memory buffer
		static D3D12_SHADER_BYTECODE FromMemory(const byte* bytecodeData, int count);

		// Use this method to load a bytecode from a memory buffer
		template<int count>
		static D3D12_SHADER_BYTECODE FromMemory(const byte(&bytecodeData)[count]);
	};

}


#pragma endregion

#pragma region ca4GImaging.h

namespace CA4G {

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

		TextureData(int width, int height, int mipMaps, int slices, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false);

		TextureData(int width, int height, int depth, DXGI_FORMAT format, byte* data, long long dataSize, bool flipY = false);

		~TextureData();

		static gObj<TextureData> CreateEmpty(int width, int height, int mipMaps, int slices, DXGI_FORMAT format);

		static gObj<TextureData> CreateEmpty(int width, int height, int depth, DXGI_FORMAT format);

		static gObj<TextureData> LoadFromFile(const char * filename);

	private:
		int pixelStride;

	};
}

#pragma endregion

#pragma region ca4GSync.h

namespace CA4G {

	class Semaphore {
		HANDLE handle;
#ifdef _DEBUG
		int currentCounter;
		int maxCount;
		bool waiting = false;
#endif
	public:

		Semaphore(int initialCount, int maxCount)
#ifdef _DEBUG
			:currentCounter(initialCount), maxCount(maxCount)
#endif
		{
			handle = CreateSemaphore(nullptr, initialCount, maxCount, nullptr);
		}
		~Semaphore() {
			CloseHandle(handle);
		}

		inline bool Wait() {
#ifdef _DEBUG
			waiting = true;
#endif
			auto result = WaitForSingleObject(handle, INFINITE);
#ifdef _DEBUG
			waiting = false;
			currentCounter--;
#endif
			return result == WAIT_OBJECT_0;
		}

		inline void Release() {
			ReleaseSemaphore(handle, 1, nullptr);
#ifdef _DEBUG
			currentCounter++;
#endif
		}
	};

	class Mutex {
		HANDLE handle;
#ifdef _DEBUG
		bool locked = false;
#endif
	public:
		inline Mutex() {
			handle = CreateMutex(nullptr, false, nullptr);
		}
		~Mutex() {
			CloseHandle(handle);
		}

		inline bool Acquire() {
#ifdef _DEBUG
			locked = true;
#endif
			return WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0;
		}

		inline void Release() {
			ReleaseMutex(handle);
#ifdef _DEBUG
			locked = false;
#endif
		}
	};

	class CountEvent {
		int count = 0;
		Mutex mutex;
		Semaphore goSemaphore;
	public:
		inline CountEvent() :mutex(Mutex()), goSemaphore(Semaphore(1, 1)) {
		}

		inline void Increment() {
			mutex.Acquire();
			if (this->count == 0)
				goSemaphore.Wait();
			this->count++;
			mutex.Release();
		}

		inline void Wait() {
			goSemaphore.Wait();
			goSemaphore.Release();
		}

		inline void Signal() {
			mutex.Acquire();
			this->count--;
			if (this->count == 0)
				goSemaphore.Release();
			mutex.Release();
		}
	};

	template<typename T>
	class ProducerConsumerQueue {
		T* elements;
		int elementsLength;
		int start;
		int count;
		Semaphore productsSemaphore;
		Semaphore spacesSemaphore;
		Mutex mutex;
		bool closed;
	public:
		ProducerConsumerQueue(int capacity) :
			productsSemaphore(Semaphore(0, capacity)),
			spacesSemaphore(Semaphore(capacity, capacity)),
			mutex(Mutex()) {
			elementsLength = capacity;
			elements = new T[elementsLength];
			start = 0;
			count = 0;
			closed = false;
		}

		~ProducerConsumerQueue() {
			delete[] elements;
		}
		inline int getCount() { return count; }

		bool TryConsume(T& element);

		bool TryProduce(T element);
	};

}

#pragma endregion

#pragma region GMath.h

#ifndef GMATH_H
#define GMATH_H

#include <cmath>

#define PI (float)3.14159265358979323846

#ifndef min
#define min(a,b) ((a)<(b)?a:b)
#define max(a,b) ((a)>(b)?a:b)
#endif

#define O_4x4 float4x4(0)
#define I_4x4 float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)

#define xyz(v) float3(v.x,v.y,v.z)

namespace CA4G {

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

	static float4 operator + (const float4 &v1, const float4 & v2)
	{
		return float4{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
	}
	static float4 operator - (const float4 & v1, const float4 & v2)
	{
		return float4{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
	}
	static float4 operator * (const float4 & v1, const float4 & v2)
	{
		return float4{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w };
	}
	static float4 operator / (const float4 & v1, const float4 & v2)
	{
		return float4{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w };
	}
	static float3 operator + (const float3 &v1, const float3 & v2)
	{
		return float3{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
	}
	static float3 operator - (const float3 & v1, const float3 & v2)
	{
		return float3{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}
	static float3 operator * (const float3 & v1, const float3 & v2)
	{
		return float3{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
	}
	static float3 operator / (const float3 & v1, const int3 & v2)
	{
		return float3{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
	}
	static float3 operator / (const float3 & v1, const float & alpha)
	{
		return float3{ v1.x / alpha, v1.y / alpha, v1.z / alpha };
	}
	static float3 operator / (const float3 & v1, const float3 & v2)
	{
		return float3{ v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
	}
	static float2 operator + (const float2 &v1, const float2 & v2)
	{
		return float2{ v1.x + v2.x, v1.y + v2.y };
	}
	static float2 operator - (const float2 & v1, const float2 & v2)
	{
		return float2{ v1.x - v2.x, v1.y - v2.y };
	}
	static float2 operator * (const float2 & v1, const float2 & v2)
	{
		return float2{ v1.x * v2.x, v1.y * v2.y };
	}
	static float2 operator / (const float2 & v1, const float2 & v2)
	{
		return float2{ v1.x / v2.x, v1.y / v2.y };
	}
	static float4x4 operator + (const float4x4 &m1, const float4x4 &m2)
	{
		return float4x4{
			m1.M00 + m2.M00,m1.M01 + m2.M01,m1.M02 + m2.M02,m1.M03 + m2.M03,
			m1.M10 + m2.M10,m1.M11 + m2.M11,m1.M12 + m2.M12,m1.M13 + m2.M13,
			m1.M20 + m2.M20,m1.M21 + m2.M21,m1.M22 + m2.M22,m1.M23 + m2.M23,
			m1.M30 + m2.M30,m1.M31 + m2.M31,m1.M32 + m2.M32,m1.M33 + m2.M33
		};
	}
	static float4x4 operator - (const float4x4 &m1, const float4x4 &m2)
	{
		return float4x4{
			m1.M00 - m2.M00,m1.M01 - m2.M01,m1.M02 - m2.M02,m1.M03 - m2.M03,
			m1.M10 - m2.M10,m1.M11 - m2.M11,m1.M12 - m2.M12,m1.M13 - m2.M13,
			m1.M20 - m2.M20,m1.M21 - m2.M21,m1.M22 - m2.M22,m1.M23 - m2.M23,
			m1.M30 - m2.M30,m1.M31 - m2.M31,m1.M32 - m2.M32,m1.M33 - m2.M33
		};
	}
	static float4x4 operator * (const float4x4 &m1, const float4x4 &m2)
	{
		return float4x4{
			m1.M00 * m2.M00,m1.M01 * m2.M01,m1.M02 * m2.M02,m1.M03 * m2.M03,
			m1.M10 * m2.M10,m1.M11 * m2.M11,m1.M12 * m2.M12,m1.M13 * m2.M13,
			m1.M20 * m2.M20,m1.M21 * m2.M21,m1.M22 * m2.M22,m1.M23 * m2.M23,
			m1.M30 * m2.M30,m1.M31 * m2.M31,m1.M32 * m2.M32,m1.M33 * m2.M33
		};
	}
	static float4x4 operator / (const float4x4 &m1, const float4x4 &m2)
	{
		return float4x4{
			m1.M00 / m2.M00,m1.M01 / m2.M01,m1.M02 / m2.M02,m1.M03 / m2.M03,
			m1.M10 / m2.M10,m1.M11 / m2.M11,m1.M12 / m2.M12,m1.M13 / m2.M13,
			m1.M20 / m2.M20,m1.M21 / m2.M21,m1.M22 / m2.M22,m1.M23 / m2.M23,
			m1.M30 / m2.M30,m1.M31 / m2.M31,m1.M32 / m2.M32,m1.M33 / m2.M33
		};
	}

	static float3x3 operator + (const float3x3 &m1, const float3x3 & m2)
	{
		return float3x3{
			m1.M00 + m2.M00,m1.M01 + m2.M01,m1.M02 + m2.M02,
			m1.M10 + m2.M10,m1.M11 + m2.M11,m1.M12 + m2.M12,
			m1.M20 + m2.M20,m1.M21 + m2.M21,m1.M22 + m2.M22
		};
	}
	static float3x3 operator - (const float3x3 & m1, const float3x3 & m2)
	{
		return float3x3{
			m1.M00 - m2.M00,m1.M01 - m2.M01,m1.M02 - m2.M02,
			m1.M10 - m2.M10,m1.M11 - m2.M11,m1.M12 - m2.M12,
			m1.M20 - m2.M20,m1.M21 - m2.M21,m1.M22 - m2.M22
		};
	}
	static float3x3 operator * (const float3x3 & m1, const float3x3 & m2)
	{
		return float3x3{
			m1.M00 * m2.M00,m1.M01 * m2.M01,m1.M02 * m2.M02,
			m1.M10 * m2.M10,m1.M11 * m2.M11,m1.M12 * m2.M12,
			m1.M20 * m2.M20,m1.M21 * m2.M21,m1.M22 * m2.M22
		};
	}
	static float3x3 operator / (const float3x3 & m1, const float3x3 & m2)
	{
		return float3x3{
			m1.M00 / m2.M00,m1.M01 / m2.M01,m1.M02 / m2.M02,
			m1.M10 / m2.M10,m1.M11 / m2.M11,m1.M12 / m2.M12,
			m1.M20 / m2.M20,m1.M21 / m2.M21,m1.M22 / m2.M22
		};
	}

	static float2x2 operator + (const float2x2 &m1, const float2x2 & m2)
	{
		return float2x2{
			m1.M00 + m2.M00,m1.M01 + m2.M01,
			m1.M10 + m2.M10,m1.M11 + m2.M11
		};
	}
	static float2x2 operator - (const float2x2 & m1, const float2x2 & m2)
	{
		return float2x2{
			m1.M00 - m2.M00,m1.M01 - m2.M01,
			m1.M10 - m2.M10,m1.M11 - m2.M11
		};
	}
	static float2x2 operator * (const float2x2 & m1, const float2x2 & m2)
	{
		return float2x2{
			m1.M00 * m2.M00,m1.M01 * m2.M01,
			m1.M10 * m2.M10,m1.M11 * m2.M11
		};
	}
	static float2x2 operator / (const float2x2 & m1, const float2x2 & m2)
	{
		return float2x2{
			m1.M00 / m2.M00,m1.M01 / m2.M01,
			m1.M10 / m2.M10,m1.M11 / m2.M11
		};
	}

	static bool any(const float3 &v) {
		return v.x != 0 || v.y != 0 || v.z != 0;
	}
	static bool any(const float4 &v) {
		return v.x != 0 || v.y != 0 || v.z != 0 || v.w != 0;
	}

	static float dot(const float4 &v1, const float4 &v2)
	{
		return v1.x *v2.x + v1.y* v2.y + v1.z *v2.z + v1.w*v2.w;
	}
	static float dot(const float3 &v1, const float3 &v2)
	{
		return v1.x *v2.x + v1.y* v2.y + v1.z *v2.z;
	}
	static float dot(const float2 &v1, const float2 &v2)
	{
		return v1.x *v2.x + v1.y* v2.y;
	}

	static float3 cross(const float3 &v1, const float3 &v2)
	{
		return float3(
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x);
	}

	static float2 lerp(const float2 &v1, const float2 &v2, const float alpha)
	{
		return float2(
			v1.x*(1 - alpha) + v2.x*alpha,
			v1.y*(1 - alpha) + v2.y*alpha);
	}
	static float3 lerp(const float3 &v1, const float3 &v2, const float alpha)
	{
		return float3(
			v1.x*(1 - alpha) + v2.x*alpha,
			v1.y*(1 - alpha) + v2.y*alpha,
			v1.z*(1 - alpha) + v2.z*alpha);
	}
	static float4 lerp(const float4 &v1, const float4 &v2, const float alpha)
	{
		return float4(
			v1.x*(1 - alpha) + v2.x*alpha,
			v1.y*(1 - alpha) + v2.y*alpha,
			v1.z*(1 - alpha) + v2.z*alpha,
			v1.w*(1 - alpha) + v2.w*alpha);
	}

	static float2x2 mul(const float2x2 &m1, const float2x2 &m2) {
		return float2x2(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)));
	}
	static float3x3 mul(const float3x3 &m1, const float3x3 &m2)
	{
		return float3x3(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)) + (m1.M02 * (m2.M20)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)) + (m1.M02 * (m2.M21)), m1.M00 * (m2.M02) + (m1.M01 * (m2.M12)) + (m1.M02 * (m2.M22)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)) + (m1.M12 * (m2.M20)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)) + (m1.M12 * (m2.M21)), m1.M10 * (m2.M02) + (m1.M11 * (m2.M12)) + (m1.M12 * (m2.M22)), m1.M20 * (m2.M00) + (m1.M21 * (m2.M10)) + (m1.M22 * (m2.M20)), m1.M20 * (m2.M01) + (m1.M21 * (m2.M11)) + (m1.M22 * (m2.M21)), m1.M20 * (m2.M02) + (m1.M21 * (m2.M12)) + (m1.M22 * (m2.M22)));
	}
	static float4x4 mul(const float4x4 &m1, const float4x4 &m2)
	{
		return float4x4(m1.M00 * (m2.M00) + (m1.M01 * (m2.M10)) + (m1.M02 * (m2.M20)) + (m1.M03 * (m2.M30)), m1.M00 * (m2.M01) + (m1.M01 * (m2.M11)) + (m1.M02 * (m2.M21)) + (m1.M03 * (m2.M31)), m1.M00 * (m2.M02) + (m1.M01 * (m2.M12)) + (m1.M02 * (m2.M22)) + (m1.M03 * (m2.M32)), m1.M00 * (m2.M03) + (m1.M01 * (m2.M13)) + (m1.M02 * (m2.M23)) + (m1.M03 * (m2.M33)), m1.M10 * (m2.M00) + (m1.M11 * (m2.M10)) + (m1.M12 * (m2.M20)) + (m1.M13 * (m2.M30)), m1.M10 * (m2.M01) + (m1.M11 * (m2.M11)) + (m1.M12 * (m2.M21)) + (m1.M13 * (m2.M31)), m1.M10 * (m2.M02) + (m1.M11 * (m2.M12)) + (m1.M12 * (m2.M22)) + (m1.M13 * (m2.M32)), m1.M10 * (m2.M03) + (m1.M11 * (m2.M13)) + (m1.M12 * (m2.M23)) + (m1.M13 * (m2.M33)), m1.M20 * (m2.M00) + (m1.M21 * (m2.M10)) + (m1.M22 * (m2.M20)) + (m1.M23 * (m2.M30)), m1.M20 * (m2.M01) + (m1.M21 * (m2.M11)) + (m1.M22 * (m2.M21)) + (m1.M23 * (m2.M31)), m1.M20 * (m2.M02) + (m1.M21 * (m2.M12)) + (m1.M22 * (m2.M22)) + (m1.M23 * (m2.M32)), m1.M20 * (m2.M03) + (m1.M21 * (m2.M13)) + (m1.M22 * (m2.M23)) + (m1.M23 * (m2.M33)), m1.M30 * (m2.M00) + (m1.M31 * (m2.M10)) + (m1.M32 * (m2.M20)) + (m1.M33 * (m2.M30)), m1.M30 * (m2.M01) + (m1.M31 * (m2.M11)) + (m1.M32 * (m2.M21)) + (m1.M33 * (m2.M31)), m1.M30 * (m2.M02) + (m1.M31 * (m2.M12)) + (m1.M32 * (m2.M22)) + (m1.M33 * (m2.M32)), m1.M30 * (m2.M03) + (m1.M31 * (m2.M13)) + (m1.M32 * (m2.M23)) + (m1.M33 * (m2.M33)));
	}
	static float2 mul(const float2 &v, const float2x2 &m) {
		return float2(v.x * m.M00 + v.y*m.M10, v.x*m.M01 + v.y*m.M11);
	}
	static float3 mul(const float3 &v, const float3x3 &m)
	{
		return float3(v.x * (m.M00) + (v.y * (m.M10)) + (v.z * (m.M20)), v.x * (m.M01) + (v.y * (m.M11)) + (v.z * (m.M21)), v.x * (m.M02) + (v.y * (m.M12)) + (v.z * (m.M22)));
	}
	static float4 mul(const float4 &v, const float4x4 &m)
	{
		return float4(v.x * (m.M00) + (v.y * (m.M10)) + (v.z * (m.M20)) + (v.w * (m.M30)), v.x * (m.M01) + (v.y * (m.M11)) + (v.z * (m.M21)) + (v.w * (m.M31)), v.x * (m.M02) + (v.y * (m.M12)) + (v.z * (m.M22)) + (v.w * (m.M32)), v.x * (m.M03) + (v.y * (m.M13)) + (v.z * (m.M23)) + (v.w * (m.M33)));
	}

	static float3 minf(float3 x, float3 y) {
		return float3(min(x.x, y.x), min(x.y, y.y), min(x.z, y.z));
	}

	static float3 maxf(float3 x, float3 y) {
		return float3(max(x.x, y.x), max(x.y, y.y), max(x.z, y.z));
	}

	static float2x2 transpose(const float2x2 &m)
	{
		return float2x2(m.M00, m.M10, m.M01, m.M11);
	}
	static float3x3 transpose(const float3x3 &m)
	{
		return float3x3(m.M00, m.M10, m.M20, m.M01, m.M11, m.M21, m.M02, m.M12, m.M22);
	}
	static float4x4 transpose(const float4x4 &m)
	{
		return float4x4(m.M00, m.M10, m.M20, m.M30, m.M01, m.M11, m.M21, m.M31, m.M02, m.M12, m.M22, m.M32, m.M03, m.M13, m.M23, m.M33);
	}

	static float length(const float2 &v)
	{
		return sqrtf(v.x*v.x + v.y*v.y);
	}
	static float length(const float3 &v)
	{
		return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
	}
	static float length(const float4 &v)
	{
		return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
	}

	static float2 normalize(const float2 &v)
	{
		float l = length(v);
		return l == 0 ? v : v * (1 / l);
	}
	static float3 normalize(const float3 &v)
	{
		float l = length(v);
		return l == 0 ? v : v * (1 / l);
	}
	static float4 normalize(const float4 &v)
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
	static float4x4 Translate(float xoffset, float yoffset, float zoffset)
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
	static float4x4 Translate(float3 vec)
	{
		return Translate(vec.x, vec.y, vec.z);
	}
	//

	// Rotations
	/// <summary>
	/// Rotation mat around Z axis
	/// </summary>
	/// <param name="alpha">value in radian for rotation</param>
	static float4x4 RotateZ(float alpha)
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
	static float4x4 RotateZGrad(float alpha)
	{
		return RotateZ(alpha * PI / 180);
	}
	/// <summary>
	/// Rotation mat around Z axis
	/// </summary>
	/// <param name="alpha">value in radian for rotation</param>
	static float4x4 RotateY(float alpha)
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
	static float4x4 RotateYGrad(float alpha)
	{
		return RotateY(alpha * PI / 180);
	}
	/// <summary>
	/// Rotation mat around Z axis
	/// </summary>
	/// <param name="alpha">value in radian for rotation</param>
	static float4x4 RotateX(float alpha)
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
	static float4x4 RotateXGrad(float alpha)
	{
		return RotateX(alpha * PI / 180);
	}
	static float4x4 Rotate(float angle, const float3 & dir)
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
	static float4x4 RotateRespectTo(const float3 &center, const float3 &direction, float angle)
	{
		return mul(Translate(center), mul(Rotate(angle, direction), Translate(center * -1.0f)));
	}
	static float4x4 RotateGrad(float angle, const float3 & dir)
	{
		return Rotate(PI * angle / 180, dir);
	}

	//

	// Scale

	static float4x4 Scale(float xscale, float yscale, float zscale)
	{
		return float4x4(
			xscale, 0, 0, 0,
			0, yscale, 0, 0,
			0, 0, zscale, 0,
			0, 0, 0, 1);
	}
	static float4x4 Scale(const float3 & size)
	{
		return Scale(size.x, size.y, size.z);
	}

	static float4x4 ScaleRespectTo(const float3 & center, const float3 & size)
	{
		return mul(mul(Translate(center), Scale(size)), Translate(center * -1));
	}
	static float4x4 ScaleRespectTo(const float3 & center, float sx, float sy, float sz)
	{
		return ScaleRespectTo(center, float3(sx, sy, sz));
	}

	//

	// Viewing

	static float4x4 LookAtLH(const float3 & camera, const float3 & target, const float3 & upVector)
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

	static float4x4 LookAtRH(const float3 & camera, const float3 & target, const float3 & upVector)
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
	static float ZnearPlane(const float4x4 &proj)
	{
		float4 pos = mul(float4(0, 0, 0, 1), proj.getInverse());
		return pos.z / pos.w;
	}

	/// <summary>
	/// Returns the far plane distance to a given projection
	/// </summary>
	/// <param name="proj">A mat structure containing the projection</param>
	/// <returns>A float value representing the distance.</returns>
	static float ZfarPlane(const float4x4 &proj)
	{
		float4 targetPos = mul(float4(0, 0, 1, 1), proj.getInverse());
		return targetPos.z / targetPos.w;
	}

	static float4x4 PerspectiveFovLH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
	{
		float h = 1.0f / tanf(fieldOfView / 2);
		float w = h * aspectRatio;

		return float4x4(
			w, 0, 0, 0,
			0, h, 0, 0,
			0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
			0, 0, -znearPlane * zfarPlane / (zfarPlane - znearPlane), 0);
	}

	static float4x4 PerspectiveFovRH(float fieldOfView, float aspectRatio, float znearPlane, float zfarPlane)
	{
		float h = 1.0f / tanf(fieldOfView / 2);
		float w = h * aspectRatio;

		return float4x4(
			w, 0, 0, 0,
			0, h, 0, 0,
			0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
			0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
	}

	static float4x4 PerspectiveLH(float width, float height, float znearPlane, float zfarPlane)
	{
		return float4x4(
			2 * znearPlane / width, 0, 0, 0,
			0, 2 * znearPlane / height, 0, 0,
			0, 0, zfarPlane / (zfarPlane - znearPlane), 1,
			0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
	}

	static float4x4 PerspectiveRH(float width, float height, float znearPlane, float zfarPlane)
	{
		return float4x4(
			2 * znearPlane / width, 0, 0, 0,
			0, 2 * znearPlane / height, 0, 0,
			0, 0, zfarPlane / (znearPlane - zfarPlane), -1,
			0, 0, znearPlane * zfarPlane / (znearPlane - zfarPlane), 0);
	}

	static float4x4 OrthoLH(float width, float height, float znearPlane, float zfarPlane)
	{
		return float4x4(
			2 / width, 0, 0, 0,
			0, 2 / height, 0, 0,
			0, 0, 1 / (zfarPlane - znearPlane), 0,
			0, 0, znearPlane / (znearPlane - zfarPlane), 1);
	}

	static float4x4 OrthoRH(float width, float height, float znearPlane, float zfarPlane)
	{
		return float4x4(
			2 / width, 0, 0, 0,
			0, 2 / height, 0, 0,
			0, 0, 1 / (znearPlane - zfarPlane), 0,
			0, 0, znearPlane / (znearPlane - zfarPlane), 1);
	}

	//
}

#endif // !GMATH_H


#pragma endregion

#pragma region List.h

namespace CA4G {

	template<typename T>
	class list
	{
		T* elements;
		int count;
		int capacity;
	public:
		list() {
			capacity = 32;
			count = 0;
			elements = new T[capacity];
		}
		list(const list<T> &other) {
			this->count = other.count;
			this->elements = new T[other.capacity];
			for (int i = 0; i < this->count; i++)
				this->elements[i] = other.elements[i];
			this->capacity = other.capacity;
		}
	public:

		/*list<T>* clone() {
			list<T>* result = new list<T>();
			result->elements = new T[count];
			result->capacity = result->count = count;
			for (int i = 0; i < count; i++)
				result->elements[i] = elements[i];
			return result;
		}*/

		gObj<list<T>> clone() {
			gObj<list<T>> result = new list<T>();
			for (int i = 0; i < count; i++)
				result->add(elements[i]);
			return result;
		}

		void reset() {
			count = 0;
		}

		list(std::initializer_list<T> initialElements) {
			capacity = max(32, initialElements.size());
			count = initialElements.size();
			elements = new T[capacity];
			for (int i = 0; i < initialElements.size(); i++)
				elements[i] = initialElements[i];
		}

		~list() {
			delete[] elements;
		}

		void add(T item) {
			if (count == capacity)
			{
				capacity = (int)(capacity*1.3);
				T* newelements = new T[capacity];
				for (int i = 0; i < count; i++)
					newelements[i] = elements[i];
				delete[] elements;
				elements = newelements;
			}
			elements[count++] = item;
		}

		inline T& operator[](int index) const {
			return elements[index];
		}

		inline T& first() const {
			return elements[0];
		}

		inline T& last() const {
			return elements[count - 1];
		}

		inline int size() const {
			return count;
		}
	};
}

#pragma endregion

#pragma region ca4GFormats.h

namespace CA4G {

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

}


#pragma endregion

#pragma region ca4GDescriptors.h

namespace CA4G
{
	// Represents a CPU free descriptor heap manager
	// Can be used to create descriptors on CPU and the allocate arranged on the GPU descriptor when resources are bound to the pipeline
	class CPUDescriptorHeapManager {
		friend Presenter;
		DX_DescriptorHeap heap;
		unsigned int size;
		int allocated = 0;
		// Linked list to reuse free slots
		struct Node {
			int index;
			Node* next;
		} *free = nullptr;
		size_t startCPU;
		Mutex mutex;
	public:
		CPUDescriptorHeapManager(DX_Device device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity);
		~CPUDescriptorHeapManager() {
			while (free != nullptr)
			{
				Node *toDelete = free;
				free = free->next;
				delete toDelete;
			}
		}
		// Allocates a new index for a resource descriptor
		int AllocateNewHandle();
		// Releases a specific index of a resource is being released.
		inline void Release(int index) { free = new Node{ index, free }; }
		inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }
		inline DX_DescriptorHeap getInnerHeap() { return heap; }
	};

	// Represents a GPU descriptor heap manager
	// Is used to allocate descriptors on GPU sequentially when resources are bound to the pipeline
	class GPUDescriptorHeapManager {
		friend Presenter;
		DX_DescriptorHeap heap;
		unsigned int size;
		size_t startCPU;
		UINT64 startGPU;

		volatile long mallocOffset = 0;
		int capacity;
		int blockCapacity;

		struct Node {
			int index;
			Node* next;
		} *free = nullptr;

	public:
		Mutex mutex;

		inline void Reset(int frameIndex) {
			mallocOffset = frameIndex * capacity / 3;
			blockCapacity = (frameIndex + 1)*capacity / 3;
		}

		long Malloc(int descriptors) {
			auto result = InterlockedExchangeAdd(&mallocOffset, descriptors);
			if (mallocOffset >= blockCapacity)
				throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, "Descriptor heap");
			return result;
		}

		long MallocPersistent() {
			int index;
			mutex.Acquire();
			if (free != nullptr) {
				index = free->index;
				Node *toDelete = free;
				free = free->next;
				delete toDelete;
			}
			else
				index = --capacity;
			mutex.Release();
			return index;
		}

		void ReleasePersistent(long index) {
			mutex.Acquire();
			free = new Node{ index, free };
			mutex.Release();
		}

		GPUDescriptorHeapManager(DX_Device device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity);

		~GPUDescriptorHeapManager() {
			while (free != nullptr)
			{
				Node *toDelete = free;
				free = free->next;
				delete toDelete;
			}
		}
		// Gets the cpu descriptor handle to access to a specific index slot of this descriptor heap
		inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUVersion(int index) { return D3D12_GPU_DESCRIPTOR_HANDLE{ startGPU + index * size }; }
		// Gets the gpu descriptor handle to access to a specific index slot of this descriptor heap
		inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }

		inline DX_DescriptorHeap getInnerHeap() { return heap; }
	};

	// Represents a global manager of all static descriptor heaps needed.
	// Static descriptors will be accessed by the application and any threaded command list manager
	// Each trheaded command list manager will have their own gpu descriptors for csu and samplers.
	class DescriptorsManager
	{
	public:
		// -- Shader visible heaps --

		// Descriptor heap for gui windows and fonts
		gObj<GPUDescriptorHeapManager> gui_csu;

		// -- GPU heaps --
		// Descriptor heap for CBV, SRV and UAV
		gObj<GPUDescriptorHeapManager> gpu_csu;
		// Descriptor heap for sampler objects
		gObj<GPUDescriptorHeapManager> gpu_smp;

		// -- CPU heaps --

		// Descriptor heap for render targets views
		gObj<CPUDescriptorHeapManager> cpu_rt;
		// Descriptor heap for depth stencil views
		gObj<CPUDescriptorHeapManager> cpu_ds;
		// Descriptor heap for cbs, srvs and uavs in CPU memory
		gObj<CPUDescriptorHeapManager> cpu_csu;
		// Descriptor heap for sampler objects in CPU memory
		gObj<CPUDescriptorHeapManager> cpu_smp;

		DescriptorsManager(DX_Device device) :
			gui_csu(new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100)),
			gpu_csu(new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 900000)),
			gpu_smp(new GPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000)),
			cpu_rt(new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1000)),
			cpu_ds(new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1000)),
			cpu_csu(new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000)),
			cpu_smp(new CPUDescriptorHeapManager(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000))
		{
		}
	};
}

#pragma endregion

#pragma region ca4GScheduler

#define process(methodName) Process(this, &decltype(dr(this))::methodName)

#define processPtr(methodName, tagID) ProcessPtr(this, &decltype(dr(this))::methodName)

namespace CA4G
{
#define ENGINE_MASK_ALL ((1 << CA4G_SUPPORTED_ENGINES) - 1)

#define ENGINE_MASK_DIRECT 1
#define ENGINE_MASK_BUNDLE 2
#define ENGINE_MASK_COMPUTE 4
#define ENGINE_MASK_COPY 8

	template<typename T>
	T dr(T* a) { return &a; }

	template<typename A>
	struct EngineType {
		static const D3D12_COMMAND_LIST_TYPE Type;
	};
	template<>
	struct EngineType<GraphicsManager> {
		static const D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	};
	template<>
	struct EngineType<DXRManager> {
		static const D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	};
	template<>
	struct EngineType<ComputeManager> {
		static const D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	};
	template<>
	struct EngineType<CopyingManager> {
		static const D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_COPY;
	};

#pragma region Managing Graphics Process

	// Represents a signal for the end of executing queues of some engines
	// Pass this object to execute pendings method in order to wait for GPU completition
	class Signal {
		friend DeviceManager;
		friend Presenter;
		friend GPUScheduler;
		long fences[CA4G_SUPPORTED_ENGINES];
		int mask = 0;
	public:
		// No signal at all
		// Only GPUScheduler may change internal data of a signal
		Signal() {}
	};

	// Represents an abstract callable member to be used as a graphics process
	struct ICallableMember
	{
		friend DeviceManager;
		friend GPUScheduler;

		virtual D3D12_COMMAND_LIST_TYPE getType() = 0;
		virtual void Call(gObj<CommandListManager> manager) = 0;
	private:
		int TagID = 0;
	};

	// Represents a callable member used as a process of a specific command list manager type (copy, compute or graphics)
	template<typename T, typename A>
	struct CallableMember : public ICallableMember {
		T* instance;
		typedef void(T::*Member)(gObj<A>);
		Member function;
		CallableMember(T* instance, Member function) : instance(instance), function(function) {
		}
		void Call(gObj<CommandListManager> manager) {
			(instance->*function)(manager.Dynamic_Cast<A>());
		}

		D3D12_COMMAND_LIST_TYPE getType() {
			return EngineType<A>::Type;
		}
	};

	// Creates a callable member for a specific instance and RTX process method
	template<typename T>
	CallableMember<T, DXRManager> Process(T* instance, typename CallableMember<T, DXRManager>::Member f) {
		return CallableMember<T, DXRManager>(instance, f);
	}

	// Creates a callable member for a specific instance and graphics process method
	template<typename T>
	CallableMember<T, GraphicsManager> Process(T* instance, typename CallableMember<T, GraphicsManager>::Member f) {
		return CallableMember<T, GraphicsManager>(instance, f);
	}

	// Creates a callable member for a specific instance and compute process method
	template<typename T>
	CallableMember<T, ComputeManager> Process(T* instance, typename CallableMember<T, ComputeManager>::Member f) {
		return  CallableMember<T, ComputeManager>(instance, f);
	}

	// Creates a callable member for a specific instance and copy process method
	template<typename T>
	CallableMember<T, CopyingManager> Process(T* instance, typename CallableMember<T, CopyingManager>::Member f) {
		return CallableMember<T, CopyingManager>(instance, f);
	}

	// Creates a pointer to a callable member for a specific instance and graphics process method
	template<typename T>
	CallableMember<T, DXRManager>* ProcessPtr(T* instance, typename CallableMember<T, DXRManager>::Member f) {
		return new CallableMember<T, DXRManager>(instance, f);
	}

	// Creates a pointer to a callable member for a specific instance and graphics process method
	template<typename T>
	CallableMember<T, GraphicsManager>* ProcessPtr(T* instance, typename CallableMember<T, GraphicsManager>::Member f) {
		return new CallableMember<T, GraphicsManager>(instance, f);
	}

	// Creates a pointer to a callable member for a specific instance and compute process method
	template<typename T>
	CallableMember<T, ComputeManager>* ProcessPtr(T* instance, typename CallableMember<T, ComputeManager>::Member f) {
		return new CallableMember<T, ComputeManager>(instance, f);
	}

	// Creates a pointer to a callable member for a specific instance and copy process method
	template<typename T>
	CallableMember<T, CopyingManager>* ProcessPtr(T* instance, typename CallableMember<T, CopyingManager>::Member f) {
		return new CallableMember<T, CopyingManager>(instance, f);
	}

	// Next macros are used for creating a process using simply the method name.
	// Instance is assumed to be this reference


#pragma endregion

#pragma region GPU Scheduler

	class GPUScheduler {
		friend DeviceManager;
		friend Presenter;

		// Struct for threading parameters
		struct GPUWorkerInfo {
			int Index;
			GPUScheduler* Scheduler;
		} *workers;

		// Represents a wrapper for command queue functionalities
		class CommandQueueManager {
			friend GPUScheduler;
			friend Presenter;

			DX_CommandQueue dxQueue;
			long fenceValue;
			DX_Fence fence;
			HANDLE fenceEvent;
			D3D12_COMMAND_LIST_TYPE type;
		public:
			CommandQueueManager(DX_Device device, D3D12_COMMAND_LIST_TYPE type);

			// Executes a command list set in this engine
			inline void Commit(DX_CommandList* cmdLists, int count) {
				dxQueue->ExecuteCommandLists(count, (ID3D12CommandList**)cmdLists);
			}

			// Send a signals through this queue
			inline long Signal() {
				dxQueue->Signal(fence, fenceValue);
				return fenceValue++;
			}

			// Triggers the event of reaching some fence value.
			inline HANDLE TriggerEvent(long rallyPoint) {
				fence->SetEventOnCompletion(rallyPoint, fenceEvent);
				return fenceEvent;
			}

		} *queues[CA4G_SUPPORTED_ENGINES];

		// Info per engine (common to all frames)
		struct PerEngineInfo {
			CommandQueueManager *queue;

			// Info per thread
			struct PerThreadInfo {
				gObj<CommandListManager> manager;
				DX_CommandList cmdList;
				bool isActive;
				// Prepares this command list for recording. This method resets the command list to use the desiredAllocator.
				void Activate(DX_CommandAllocator desiredAllocator);

				// Prepares this cmd list for flushing to the GPU
				void Close() {
					if (!isActive)
						return;
					isActive = false;
					cmdList->Close();
				}
			} *threadInfos;

			// Info per frame and per engine
			struct PerFrameInfo {
				DX_CommandAllocator* allocatorSet;
				bool* allocatorsUsed;

				inline DX_CommandAllocator RequireAllocator(int index) {
					allocatorsUsed[index] = true;
					return allocatorSet[index];
				}

				void ResetUsedAllocators(int threads);
			} *frames;

		} engines[CA4G_SUPPORTED_ENGINES];

		// Used by flush method to call execute command list for each engine
		DX_CommandList* __activeCmdLists;

		gObj<DeviceManager> manager;
		HANDLE* threads;
		ProducerConsumerQueue<ICallableMember*>* workToDo;
		CountEvent *counting;
		int threadsCount;
		int currentFrameIndex;
		// Gets whenever the user wants to synchronize frame rendering using frame buffering
		bool useFrameBuffer;

		Signal* perFrameFinishedSignal;

		static DWORD WINAPI __WORKER_TODO(LPVOID param);

		GPUScheduler(gObj<DeviceManager> manager, bool useFrameBuffer, int max_threads = CA4G_MAX_NUMBER_OF_WORKERS, int buffers = CA4G_SUPPORTED_BUFFERING);

		void PopulateCommandsBy(ICallableMember *process, int workerIndex);

		bool _closed = false;

		void Terminate() {
			_closed = true;
		}

		// Gets when this scheduler is terminated
		inline bool isClosed() {
			return _closed;
		}

		// Commit all pending works at specific engines and returns a mask with the active engines.
		int Flush(int mask);

		// Sends a signal throught the specific engines and returns the Signal object with all fence values
		Signal SendSignal(int mask) {
			Signal s;
			for (int e = 0; e < CA4G_SUPPORTED_ENGINES; e++)
				if (!((1 << e) & ENGINE_MASK_BUNDLE))
					if (mask & (1 << e))
						s.fences[e] = queues[e]->Signal();
			s.mask = mask;
			return s;
		}

		HANDLE __fencesForWaiting[CA4G_SUPPORTED_ENGINES];

		// Waits for the gpu notification of certain signal
		void WaitFor(const Signal &signal) {
			int fencesForWaiting = 0;
			for (int e = 0; e < CA4G_SUPPORTED_ENGINES; e++)
				if (!((1 << e) & ENGINE_MASK_BUNDLE))
					if (signal.mask & (1 << e))
						__fencesForWaiting[fencesForWaiting++] = queues[e]->TriggerEvent(signal.fences[e]);
			WaitForMultipleObjects(fencesForWaiting, __fencesForWaiting, true, INFINITE);
		}

		void SetupFrame(int frame) {
			if (useFrameBuffer)
				WaitFor(perFrameFinishedSignal[frame]); // Grants the GPU finished working this frame in a previous stage

			currentFrameIndex = frame;

			for (int e = 0; e < CA4G_SUPPORTED_ENGINES; e++)
				engines[e].frames[frame].ResetUsedAllocators(threadsCount);
		}

		void FinishFrame() {
			perFrameFinishedSignal[currentFrameIndex] = SendSignal(ENGINE_MASK_ALL);

			if (!useFrameBuffer)
				// Grants the GPU finished working this frame before finishing this frame
				WaitFor(perFrameFinishedSignal[currentFrameIndex]);
		}

		void Enqueue(ICallableMember* process) {
			PopulateCommandsBy(process, 0); // worker 0 is the synchronous worker (main thread)
		}

		void EnqueueAsync(ICallableMember *process) {
			counting->Increment();
			workToDo->TryProduce(process);
		}
	};

#pragma endregion
}

#pragma endregion

#pragma region ca4GResources_Private.h

namespace CA4G {
	// Represents the accessibility optimization of a resource by the cpu
	typedef enum CPU_ACCESS {
		// The CPU can not read or write
		CPU_ACCESS_NONE = 0,
		// The CPU can write and GPU can access frequently efficiently
		CPU_WRITE_GPU_READ = 1,
		// The GPU can write once and CPU can access frequently efficiently
		CPU_READ_GPU_WRITE = 2
	} CPU_ACCESS;

	// Gets the total number of bytes required by a resource with this description
	inline UINT64 GetRequiredIntermediateSize(DX_Device device, const D3D12_RESOURCE_DESC &desc)
	{
		UINT64 RequiredSize = 0;
		device->GetCopyableFootprints(&desc, 0, desc.DepthOrArraySize*desc.MipLevels, 0, nullptr, nullptr, nullptr, &RequiredSize);
		return RequiredSize;
	}

	// Represents a wrapper to a DX12 resource. This class is for internal purpose only. Users access directly always via a resource view.
	class ResourceWrapper {
		friend Creating; //allow to access to resource wrapper private constructor for resource creation
		friend Presenter; // allow to access to resource wrapper private constructor for presented render targets
		friend ResourceView; // allow to reference counting

	private:

		// Creates a wrapper to a resource with a description and an initial state
		ResourceWrapper(gObj<DeviceManager> manager, D3D12_RESOURCE_DESC description, DX_Resource resource, D3D12_RESOURCE_STATES initialState);

		int subresources;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts;
		unsigned int* pNumRows;
		UINT64 *pRowSizesInBytes;
		UINT64 pTotalSizes;
		Mutex mutex;
		// How many resource views are referencing this resource. This resource is automatically released when no view is referencing it.
		int references = 0;


	public:
		~ResourceWrapper() {
			delete[] pLayouts;
			delete[] pNumRows;
			delete[] pRowSizesInBytes;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() {
			return internalResource->GetGPUVirtualAddress();
		}
		// Device Manager to manage this resource
		gObj<DeviceManager> manager;
		// Resource
		DX_Resource internalResource;
		// Abstract resource description
		D3D12_RESOURCE_DESC desc;
		// Last used state of this resource on the GPU
		D3D12_RESOURCE_STATES LastUsageState;

		// Gets the cpu accessibility of this resource wrapper.
		CPU_ACCESS cpu_access;

		// Cached version of this resource for uploading (CPU-to-GPU) purposes.
		gObj<ResourceWrapper> uploadingResourceCache = nullptr; //cache for copying to GPU
		// Cached version of this resource for downloading (GPU-to-CPU) purposes.
		gObj<ResourceWrapper> downloadingResourceCache = nullptr; //cache for copying from GPU

		// If this resource is in upload heap, this is a ptr to the mapped data (to prevent map/unmap overhead)
		void* mappedData = nullptr;

		void UpdateMappedData(int position, void* data, int size);

		byte* GetMappedDataAddress();

		// Prepares this resource wrapper to have an uploading version
		void CreateForUploading();

		//---Copied from d3d12x.hs----------------------------------------------------------------------------
		// Row-by-row memcpy
		inline void MemcpySubresource(
			_In_ const D3D12_MEMCPY_DEST* pDest,
			_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
			SIZE_T RowSizeInBytes,
			UINT NumRows,
			UINT NumSlices,
			bool flipRows = false
		)
		{
			for (UINT z = 0; z < NumSlices; ++z)
			{
				BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
				const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
				for (UINT y = 0; y < NumRows; ++y)
				{
					memcpy(pDestSlice + pDest->RowPitch * y,
						pSrcSlice + pSrc->RowPitch * (flipRows ? NumRows - y - 1 : y),
						RowSizeInBytes);
				}
			}
		}

		// Uploads a full data mapped of this resource.
		// All data (for all subresources if any) appears sequentially in ptr buffer.
		void UploadFullData(byte* data, long long dataSize, bool flipRows = false);

		// Uploads the data of a region of a single subresource.
		template<typename T>
		void Upload(T* data, int subresource = 0, const D3D12_BOX *box = nullptr);
	};


}

#pragma endregion

#pragma region ca4GResources.h

namespace CA4G {

	// Represents a sampler object
	struct Sampler {
		D3D12_FILTER Filter;
		D3D12_TEXTURE_ADDRESS_MODE AddressU;
		D3D12_TEXTURE_ADDRESS_MODE AddressV;
		D3D12_TEXTURE_ADDRESS_MODE AddressW;
		FLOAT MipLODBias;
		UINT MaxAnisotropy;
		D3D12_COMPARISON_FUNC ComparisonFunc;
		float4 BorderColor;
		FLOAT MinLOD;
		FLOAT MaxLOD;

		// Creates a default point sampling object.
		static Sampler Point(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_POINT,
				AddressU,
				AddressV,
				AddressW);
		}

		// Creates a default point sampling object.
		static Sampler PointWithoutMipMaps(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_POINT,
				AddressU,
				AddressV,
				AddressW, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS, float4(0,0,0,0), 0, 0);
		}

		// Creates a default linear sampling object
		static Sampler Linear(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				AddressU,
				AddressV,
				AddressW);
		}

		static Sampler LinearWithoutMipMaps(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER
		)
		{
			return Create(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
				AddressU,
				AddressV,
				AddressW, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS, float4(0, 0, 0, 0), 0, 0);
		}

		// Creates a default anisotropic sampling object
		static Sampler Anisotropic(
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP
		)
		{
			return Create(D3D12_FILTER_ANISOTROPIC,
				AddressU,
				AddressV,
				AddressW);
		}
	private:
		static Sampler Create(
			D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			float mipLODBias = 0.0f,
			UINT maxAnisotropy = 16,
			D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
			float4 borderColor = float4(0, 0, 0, 0),
			float minLOD = 0,
			float maxLOD = D3D12_FLOAT32_MAX
		) {
			return Sampler{
				filter,
				AddressU,
				AddressV,
				AddressW,
				mipLODBias,
				maxAnisotropy,
				comparisonFunc,
				borderColor,
				minLOD,
				maxLOD
			};
		}
	};

	// Represents a resource view. In this engine resources are treated always through some view.
	// Although, views can be cast from one type to another.
	class ResourceView {
		friend Presenter;
		friend CommandListManager;
		friend Creating;
		friend Copying;
		template <typename ...A> friend class PipelineBindings;
		friend GraphicsManager;
		friend DeviceManager;
		friend GeometryCollection;
		friend TriangleGeometryCollection;
		friend InstanceCollection;
		friend IRTProgram;

		// Used for binding and other scenarios to synchronize the usage of the resource
		// barriering the states changes.
		void ChangeStateTo(DX_CommandList cmdList, D3D12_RESOURCE_STATES dst) {
			if (resource->LastUsageState == dst)
				return;

			D3D12_RESOURCE_BARRIER barrier = { };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource->internalResource;
			barrier.Transition.StateAfter = dst;
			barrier.Transition.StateBefore = resource->LastUsageState;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1, &barrier);
			resource->LastUsageState = dst;
		}

		void ChangeStateToUAV(DX_CommandList cmdList) {
			D3D12_RESOURCE_BARRIER barrier = { };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.UAV.pResource = this->resource->internalResource;
			cmdList->ResourceBarrier(1, &barrier);
			resource->LastUsageState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		void ChangeStateFromTo(DX_CommandList cmdList, D3D12_RESOURCE_STATES src, D3D12_RESOURCE_STATES dst) {
			if (resource->LastUsageState == dst)
				return;

			D3D12_RESOURCE_BARRIER barrier = { };
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource->internalResource;
			barrier.Transition.StateAfter = dst;
			barrier.Transition.StateBefore = resource->LastUsageState;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1, &barrier);
			resource->LastUsageState = dst;
		}

		// This is a special ResourceView instance representing the creation of a null descriptor.
		// This instancing is using only for binding null purposes.
		static gObj<ResourceView> getNullView(gObj<DeviceManager> manager, D3D12_RESOURCE_DIMENSION dimension);


	protected:
		// Gets the internal resource this view is representing to.
		gObj<ResourceWrapper> resource = nullptr;
		gObj<DeviceManager> manager;

		// Slice that represents the subresources this view refers to
		int mipSliceStart = 0;
		int mipSliceCount = 1;
		int arraySliceStart = 0;
		int arraySliceCount = 1;

		// Cached CPU descriptor handles (each descriptor for view is stored in a single CPU slot)
		int srv; // 1
		int uav; // 2
		int cbv; // 4
		int rtv; // 8
		int dsv; // 16
		// Mask representing the slots of the cpu descriptor this resource has a descriptor view built at.
		unsigned int handleMask = 0;

		Mutex mutex;

		virtual void CreateUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC &d) = 0;
		virtual void CreateSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC &d) = 0;
		virtual void CreateCBVDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC &d) = 0;
		virtual void CreateRTVDesc(D3D12_RENDER_TARGET_VIEW_DESC &d) = 0;
		virtual void CreateDSVDesc(D3D12_DEPTH_STENCIL_VIEW_DESC &d) = 0;

		ResourceView(gObj<ResourceWrapper> resource, int arrayStart = 0, int arrayCount = INT_MAX, int mipStart = 0, int mipsCount = INT_MAX)
			: resource(resource), manager(resource->manager)
		{
			resource->references++;

			if (resource->desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
				this->mipSliceStart = mipStart;
				this->mipSliceCount = min(resource->desc.MipLevels, mipsCount);
				this->arraySliceStart = arrayStart;
				this->arraySliceCount = min(resource->desc.DepthOrArraySize, arrayCount);
			}
		}

		// Creates a NULL RESOURCE VIEW
		ResourceView(gObj<DeviceManager> manager) :manager(manager) {

		}

		int getSRV();

		D3D12_CPU_DESCRIPTOR_HANDLE getSRVHandle();

		int getUAV();

		D3D12_CPU_DESCRIPTOR_HANDLE getUAVHandle();

		int getCBV();

		D3D12_CPU_DESCRIPTOR_HANDLE getCBVHandle();

		int getRTV();

		D3D12_CPU_DESCRIPTOR_HANDLE getRTVHandle();

		int getDSV();

		D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle();

		void getCPUHandleFor(D3D12_DESCRIPTOR_RANGE_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE &handle) {
			switch (type) {
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
				handle = getCBVHandle();
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
				handle = getSRVHandle();
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				handle = getUAVHandle();
				break;
			}
		}

		void getCPUHandleFor(D3D12_ROOT_PARAMETER_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE &handle) {
			switch (type) {
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
				handle = getCBVHandle();
				break;
			case D3D12_ROOT_PARAMETER_TYPE_SRV:
				handle = getSRVHandle();
				break;
			case D3D12_ROOT_PARAMETER_TYPE_UAV:
				handle = getUAVHandle();
				break;
			}
		}


	public:

		~ResourceView();

		// Cast this resource to a different view type.
		template<typename V>
		V* CreateCastView()
		{
			return new V(this->resource, this->arraySliceStart, this->arraySliceCount, this->mipSliceStart, this->mipSliceCount);
		}
	};

	// Represents a buffer view of a resource.
	class Buffer : public ResourceView {
		friend Creating;
		friend Presenter;
		friend GraphicsManager;
		friend ResourceView;
		friend DXRManager;
		friend SceneBuilder;
		friend RTPipelineManager;
		friend IRTProgram;
		friend TriangleGeometryCollection;
		friend ProceduralGeometryCollection;
		friend GeometryCollection;
		friend InstanceCollection;

		byte* GetShaderRecordStartAddress(int index) {
			return ((byte*)resource->GetMappedDataAddress()) + index * Stride;
		}

	protected:
		// Inherited via Resource
		void CreateUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC & d)
		{
			d.Buffer.CounterOffsetInBytes = 0;
			d.Buffer.FirstElement = 0;
			d.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			d.Buffer.NumElements = ElementCount;
			d.Buffer.StructureByteStride = Stride;
			d.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			d.Format = DXGI_FORMAT_UNKNOWN;// !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC & d)
		{
			d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			d.Buffer.FirstElement = 0;
			d.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			d.Buffer.NumElements = ElementCount;
			d.Buffer.StructureByteStride = Stride;
			d.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateCBVDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC & d)
		{
			d.BufferLocation = !resource ? 0 : resource->internalResource->GetGPUVirtualAddress();
			d.SizeInBytes = (Stride * ElementCount + 255)&~255;
		}

		void CreateRTVDesc(D3D12_RENDER_TARGET_VIEW_DESC & d)
		{
			throw "Not supported a buffer as render target";
		}

		void CreateDSVDesc(D3D12_DEPTH_STENCIL_VIEW_DESC & d)
		{
			throw "Not supported a buffer as depth stencil view";
		}

		void CreateVBV(D3D12_VERTEX_BUFFER_VIEW &v) {
			v.BufferLocation = !resource ? 0 : resource->internalResource->GetGPUVirtualAddress();
			v.SizeInBytes = ElementCount * Stride;
			v.StrideInBytes = Stride;
		}

		void CreateIBV(D3D12_INDEX_BUFFER_VIEW &i) {
			i.BufferLocation = !resource ? 0 : resource->internalResource->GetGPUVirtualAddress();
			i.Format = !resource ? DXGI_FORMAT_UNKNOWN : Stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			i.SizeInBytes = ElementCount * Stride;
		}

		Buffer(gObj<ResourceWrapper> resource, int stride)
			: ResourceView(resource), ElementCount(resource->desc.Width / stride), Stride(stride)
		{
		}

		Buffer(gObj<DeviceManager> manager) : ResourceView(manager), ElementCount(1), Stride(256)
		{
		}

	public:
		// Number of elements of this buffer
		unsigned int const
			ElementCount;
		// Number of bytes of each element in this buffer
		unsigned int const
			Stride;
	};

	// Represents a 2D view of a resource. This view includes texture arrays, mipmaps and planes.
	class Texture2D : public ResourceView {
		friend Creating;
		friend Presenter;
		friend ResourceView;
		friend CommandListManager;

	protected:
		// Inherited via Resource
		void CreateUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC & d)
		{
			d.Texture2DArray.ArraySize = arraySliceCount;
			d.Texture2DArray.FirstArraySlice = arraySliceStart;
			d.Texture2DArray.MipSlice = mipSliceStart;
			d.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC & d)
		{
			d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			d.Texture2DArray.ArraySize = arraySliceCount;
			d.Texture2DArray.FirstArraySlice = arraySliceStart;
			d.Texture2DArray.MipLevels = mipSliceCount;
			d.Texture2DArray.MostDetailedMip = mipSliceStart;
			d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_R8G8B8A8_UNORM : resource->desc.Format;
		}

		void CreateCBVDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC & d)
		{
			throw "Not supported convert a texture 2D into a constant buffer";
		}

		void CreateRTVDesc(D3D12_RENDER_TARGET_VIEW_DESC & d)
		{
			d.Texture2DArray.ArraySize = arraySliceCount;
			d.Texture2DArray.FirstArraySlice = arraySliceStart;
			d.Texture2DArray.MipSlice = mipSliceStart;
			d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		void CreateDSVDesc(D3D12_DEPTH_STENCIL_VIEW_DESC & d)
		{
			d.Texture2DArray.ArraySize = arraySliceCount;
			d.Texture2DArray.FirstArraySlice = arraySliceStart;
			d.Texture2DArray.MipSlice = mipSliceStart;
			d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
		}

		Texture2D(gObj<ResourceWrapper> resource, int arrayStart = 0, int arrayCount = INT_MAX, int mipStart = 0, int mipsCount = INT_MAX)
			: ResourceView(resource, arrayStart, arrayCount, mipStart, mipsCount)
			, Width(max(1, resource->desc.Width >> mipStart)), Height(max(1, resource->desc.Height >> mipStart))
		{
		}

		Texture2D(gObj<DeviceManager> manager) : ResourceView(manager), Width(1), Height(1) {
		}
	public:
		// Width of this Texture2D
		int const Width;
		// Height of this Texture2D
		int const Height;

		inline int getMipsCount() {
			return mipSliceCount;
		}
		inline int getSlicesCount() {
			return arraySliceCount;
		}
		gObj<Texture2D> CreateSlice(int sliceStart = 0, int sliceCount = INT_MAX, int mipStart = 0, int mipCount = INT_MAX) {
			return new Texture2D(this->resource, this->arraySliceStart + sliceStart, sliceCount, this->mipSliceStart + mipStart, mipCount);
		}
		gObj<Texture2D> CreateSubresource(int sliceStart = 0, int mipStart = 0) {
			return CreateSlice(sliceStart, 1, mipStart, 1);
		}
		gObj<Texture2D> CreateArraySlice(int slice) {
			return CreateSlice(slice, 1);
		}
		gObj<Texture2D> CreateMipSlice(int slice) {
			return CreateSlice(arraySliceStart, arraySliceCount, slice, 1);
		}
	};

}

#pragma endregion

#pragma region ca4GPipelineBindings.h

#pragma region Compiled Shaders

#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// POSITION                 0   xy          0     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float   xyzw
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
vs_5_0
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_input v0.xy
dcl_output_siv o0.xyzw, position
dcl_output o1.xy
dcl_temps 1
//
// Initial variable locations:
//   v0.x <- pos.x; v0.y <- pos.y; 
//   o0.x <- <main return value>.x; o0.y <- <main return value>.y; o0.z <- <main return value>.z; o0.w <- <main return value>.w; 
//   o1.x <- C.x; o1.y <- C.y
//
#line 3 "C:\Users\lleonart\Desktop\PhD Project\CA4G\Shaders\DrawScreen_VS.hlsl"
itof r0.x, l(1)
add r0.x, r0.x, v0.x
mul o1.x, r0.x, l(0.500000)
itof r0.x, l(1)
mov r0.y, -v0.y
add r0.x, r0.y, r0.x
mul o1.y, r0.x, l(0.500000)

#line 4
itof o0.w, l(1)
mov o0.xy, v0.xyxx
mov o0.z, l(0.500000)
ret
// Approximately 11 instruction slots used
#endif

const BYTE cso_DrawScreen_VS[] =
{
	 68,  88,  66,  67,  13, 138,
	194, 211,  30, 104, 182, 169,
	 70,  65, 168, 232, 119, 129,
	165, 231,   1,   0,   0,   0,
	  8,  49,   0,   0,   6,   0,
	  0,   0,  56,   0,   0,   0,
	164,   0,   0,   0, 216,   0,
	  0,   0,  48,   1,   0,   0,
	100,   2,   0,   0,   0,   3,
	  0,   0,  82,  68,  69,  70,
	100,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  60,   0,
	  0,   0,   0,   5, 254, 255,
	  5,   1,   0,   0,  60,   0,
	  0,   0,  82,  68,  49,  49,
	 60,   0,   0,   0,  24,   0,
	  0,   0,  32,   0,   0,   0,
	 40,   0,   0,   0,  36,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   0,   0,   0,  77, 105,
	 99, 114, 111, 115, 111, 102,
	116,  32,  40,  82,  41,  32,
	 72,  76,  83,  76,  32,  83,
	104,  97, 100, 101, 114,  32,
	 67, 111, 109, 112, 105, 108,
	101, 114,  32,  49,  48,  46,
	 49,   0,  73,  83,  71,  78,
	 44,   0,   0,   0,   1,   0,
	  0,   0,   8,   0,   0,   0,
	 32,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,   3,   3,   0,   0,
	 80,  79,  83,  73,  84,  73,
	 79,  78,   0, 171, 171, 171,
	 79,  83,  71,  78,  80,   0,
	  0,   0,   2,   0,   0,   0,
	  8,   0,   0,   0,  56,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	 15,   0,   0,   0,  68,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   1,   0,   0,   0,
	  3,  12,   0,   0,  83,  86,
	 95,  80,  79,  83,  73,  84,
	 73,  79,  78,   0,  84,  69,
	 88,  67,  79,  79,  82,  68,
	  0, 171, 171, 171,  83,  72,
	 69,  88,  44,   1,   0,   0,
	 80,   0,   1,   0,  75,   0,
	  0,   0, 106, 136,   0,   1,
	 95,   0,   0,   3,  50,  16,
	 16,   0,   0,   0,   0,   0,
	103,   0,   0,   4, 242,  32,
	 16,   0,   0,   0,   0,   0,
	  1,   0,   0,   0, 101,   0,
	  0,   3,  50,  32,  16,   0,
	  1,   0,   0,   0, 104,   0,
	  0,   2,   1,   0,   0,   0,
	 43,   0,   0,   5,  18,   0,
	 16,   0,   0,   0,   0,   0,
	  1,  64,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   7,
	 18,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  10,  16,
	 16,   0,   0,   0,   0,   0,
	 56,   0,   0,   7,  18,  32,
	 16,   0,   1,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,   1,  64,   0,   0,
	  0,   0,   0,  63,  43,   0,
	  0,   5,  18,   0,  16,   0,
	  0,   0,   0,   0,   1,  64,
	  0,   0,   1,   0,   0,   0,
	 54,   0,   0,   6,  34,   0,
	 16,   0,   0,   0,   0,   0,
	 26,  16,  16, 128,  65,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   7,  18,   0,
	 16,   0,   0,   0,   0,   0,
	 26,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  56,   0,
	  0,   7,  34,  32,  16,   0,
	  1,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	  1,  64,   0,   0,   0,   0,
	  0,  63,  43,   0,   0,   5,
	130,  32,  16,   0,   0,   0,
	  0,   0,   1,  64,   0,   0,
	  1,   0,   0,   0,  54,   0,
	  0,   5,  50,  32,  16,   0,
	  0,   0,   0,   0,  70,  16,
	 16,   0,   0,   0,   0,   0,
	 54,   0,   0,   5,  66,  32,
	 16,   0,   0,   0,   0,   0,
	  1,  64,   0,   0,   0,   0,
	  0,  63,  62,   0,   0,   1,
	 83,  84,  65,  84, 148,   0,
	  0,   0,  11,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   3,   0,   0,   0,
	  5,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  2,   0,   0,   0,   0,   0,
	  0,   0,   3,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 83,  80,  68,  66,   0,  46,
	  0,   0,  77, 105,  99, 114,
	111, 115, 111, 102, 116,  32,
	 67,  47,  67,  43,  43,  32,
	 77,  83,  70,  32,  55,  46,
	 48,  48,  13,  10,  26,  68,
	 83,   0,   0,   0,   0,   2,
	  0,   0,   2,   0,   0,   0,
	 23,   0,   0,   0, 132,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 192, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	 56,   0, 128, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255,   5,   0,   0,   0,
	 32,   0,   0,   0,  60,   0,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255,   0,   0,
	  0,   0,   6,   0,   0,   0,
	  5,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	148,  46,  49,   1, 251, 103,
	166,  92,   1,   0,   0,   0,
	 89,  73, 115,  61, 234,  36,
	100,  75, 169,  34, 138, 198,
	225, 156,  23, 109,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 220,  81,  51,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  68,  51,  68,  83,
	 72,  68,  82,   0,  44,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  32,   0,   0,  96,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 117, 131,
	  1,   0, 242, 228,   1,   0,
	198,  90,   0,   0, 229,  18,
	  2,   0, 182,  61,   0,   0,
	  0,  16,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	102, 108, 111,  97, 116,  52,
	 32, 109,  97, 105, 110,  40,
	102, 108, 111,  97, 116,  50,
	 32, 112, 111, 115,  32,  58,
	 32,  80,  79,  83,  73,  84,
	 73,  79,  78,  44,  32, 111,
	117, 116,  32, 102, 108, 111,
	 97, 116,  50,  32,  67,  32,
	 58,  32,  84,  69,  88,  67,
	 79,  79,  82,  68,  41,  32,
	 58,  32,  83,  86,  95,  80,
	 79,  83,  73,  84,  73,  79,
	 78,  13,  10, 123,  13,  10,
	  9,  67,  32,  61,  32, 102,
	108, 111,  97, 116,  50,  40,
	 40, 112, 111, 115,  46, 120,
	 32,  43,  32,  49,  41,  42,
	 48,  46,  53,  44,  32,  40,
	 49,  32,  45,  32, 112, 111,
	115,  46, 121,  41,  42,  48,
	 46,  53,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 102, 108, 111,  97,
	116,  52,  40, 112, 111, 115,
	 46, 120,  44,  32, 112, 111,
	115,  46, 121,  44,  32,  48,
	 46,  53,  44,  32,  49,  41,
	 59,  13,  10, 125,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 254, 239, 254, 239,
	  1,   0,   0,   0,  53,   1,
	  0,   0,   0,  67,  58,  92,
	 85, 115, 101, 114, 115,  92,
	108, 108, 101, 111, 110,  97,
	114, 116,  92,  68, 101, 115,
	107, 116, 111, 112,  92,  80,
	104,  68,  32,  80, 114, 111,
	106, 101,  99, 116,  92,  67,
	 65,  52,  71,  92,  83, 104,
	 97, 100, 101, 114, 115,  92,
	 68, 114,  97, 119,  83,  99,
	114, 101, 101, 110,  95,  86,
	 83,  46, 104, 108, 115, 108,
	  0,   0,  99,  58,  92, 117,
	115, 101, 114, 115,  92, 108,
	108, 101, 111, 110,  97, 114,
	116,  92, 100, 101, 115, 107,
	116, 111, 112,  92, 112, 104,
	100,  32, 112, 114, 111, 106,
	101,  99, 116,  92,  99,  97,
	 52, 103,  92, 115, 104,  97,
	100, 101, 114, 115,  92, 100,
	114,  97, 119, 115,  99, 114,
	101, 101, 110,  95, 118, 115,
	 46, 104, 108, 115, 108,   0,
	102, 108, 111,  97, 116,  52,
	 32, 109,  97, 105, 110,  40,
	102, 108, 111,  97, 116,  50,
	 32, 112, 111, 115,  32,  58,
	 32,  80,  79,  83,  73,  84,
	 73,  79,  78,  44,  32, 111,
	117, 116,  32, 102, 108, 111,
	 97, 116,  50,  32,  67,  32,
	 58,  32,  84,  69,  88,  67,
	 79,  79,  82,  68,  41,  32,
	 58,  32,  83,  86,  95,  80,
	 79,  83,  73,  84,  73,  79,
	 78,  13,  10, 123,  13,  10,
	  9,  67,  32,  61,  32, 102,
	108, 111,  97, 116,  50,  40,
	 40, 112, 111, 115,  46, 120,
	 32,  43,  32,  49,  41,  42,
	 48,  46,  53,  44,  32,  40,
	 49,  32,  45,  32, 112, 111,
	115,  46, 121,  41,  42,  48,
	 46,  53,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 102, 108, 111,  97,
	116,  52,  40, 112, 111, 115,
	 46, 120,  44,  32, 112, 111,
	115,  46, 121,  44,  32,  48,
	 46,  53,  44,  32,  49,  41,
	 59,  13,  10, 125,   0,   7,
	  0,   0,   0,   0,   0,   0,
	  0,  71,   0,   0,   0, 142,
	  0,   0,   0,   0,   0,   0,
	  0,  72,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   4,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  27, 226,
	 48,   1, 128,   0,   0,   0,
	  7,  15, 215, 102,  36, 235,
	212,   1,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  2,   0,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,  72,   0,
	  0,   0,  40,   0,   0,   0,
	 27, 226,  48,   1, 219, 168,
	 71,   1, 166,   0,   0,   0,
	  1,   0,   0,   0,  71,   0,
	  0,   0,  72,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  4,   0,   0,   0,  66,   0,
	 60,  17,  16,   1,   0,   0,
	  0,   1,  10,   0,   1,   0,
	132,   0,  99,  69,  10,   0,
	  1,   0, 132,   0,  99,  69,
	 77, 105,  99, 114, 111, 115,
	111, 102, 116,  32,  40,  82,
	 41,  32,  72,  76,  83,  76,
	 32,  83, 104,  97, 100, 101,
	114,  32,  67, 111, 109, 112,
	105, 108, 101, 114,  32,  49,
	 48,  46,  49,   0,   0,   0,
	 54,   0,  61,  17,   1, 104,
	108, 115, 108,  70, 108,  97,
	103, 115,   0,  48, 120,  53,
	  0, 104, 108, 115, 108,  84,
	 97, 114, 103, 101, 116,   0,
	118, 115,  95,  53,  95,  48,
	  0, 104, 108, 115, 108,  69,
	110, 116, 114, 121,   0, 109,
	 97, 105, 110,   0,   0,   0,
	  0,   0,  42,   0,  16,  17,
	  0,   0,   0,   0,   0,   2,
	  0,   0,   0,   0,   0,   0,
	240,   0,   0,   0,   0,   0,
	  0,   0, 240,   0,   0,   0,
	  4,  16,   0,   0,  60,   0,
	  0,   0,   1,   0, 160, 109,
	 97, 105, 110,   0,  42,   0,
	 62,  17,   0,  16,   0,   0,
	  9,   0, 112, 111, 115,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   1,   0,
	  5,   0,   0,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   1,   0,
	  5,   0,   4,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,   4,   0,   0,   0,
	 58,   0,  62,  17,   3,  16,
	  0,   0, 136,   0,  60, 109,
	 97, 105, 110,  32, 114, 101,
	116, 117, 114, 110,  32, 118,
	 97, 108, 117, 101,  62,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   2,   0,
	  5,   0,   0,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   2,   0,
	  5,   0,   4,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,   4,   0,   0,   0,
	 22,   0,  80,  17,   2,   0,
	  5,   0,   8,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,   8,   0,   0,   0,
	 22,   0,  80,  17,   2,   0,
	  5,   0,  12,   0,   4,   0,
	 60,   0,   0,   0,   1,   0,
	240,   0,  12,   0,   0,   0,
	 42,   0,  62,  17,   0,  16,
	  0,   0,   9,   0,  67,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  2,   0,   5,   0,   0,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 240,   0,  16,   0,
	  0,   0,  22,   0,  80,  17,
	  2,   0,   5,   0,   4,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 240,   0,  20,   0,
	  0,   0,   2,   0,   6,   0,
	244,   0,   0,   0,  24,   0,
	  0,   0,   1,   0,   0,   0,
	 16,   1, 119, 215,   1,  38,
	152, 235,  85, 238,  17, 147,
	 85, 162, 251, 248,  73,  96,
	  0,   0, 242,   0,   0,   0,
	 32,   1,   0,   0,   0,   0,
	  0,   0,   1,   0,   1,   0,
	 44,   1,   0,   0,   0,   0,
	  0,   0,  22,   0,   0,   0,
	 20,   1,   0,   0,  60,   0,
	  0,   0,   3,   0,   0, 128,
	 60,   0,   0,   0,   3,   0,
	  0,   0,  80,   0,   0,   0,
	  3,   0,   0, 128,  80,   0,
	  0,   0,   3,   0,   0,   0,
	108,   0,   0,   0,   3,   0,
	  0, 128, 108,   0,   0,   0,
	  3,   0,   0,   0, 136,   0,
	  0,   0,   3,   0,   0, 128,
	136,   0,   0,   0,   3,   0,
	  0,   0, 156,   0,   0,   0,
	  3,   0,   0, 128, 156,   0,
	  0,   0,   3,   0,   0,   0,
	180,   0,   0,   0,   3,   0,
	  0, 128, 180,   0,   0,   0,
	  3,   0,   0,   0, 208,   0,
	  0,   0,   3,   0,   0, 128,
	208,   0,   0,   0,   3,   0,
	  0,   0, 236,   0,   0,   0,
	  4,   0,   0, 128, 236,   0,
	  0,   0,   4,   0,   0,   0,
	  0,   1,   0,   0,   4,   0,
	  0, 128,   0,   1,   0,   0,
	  4,   0,   0,   0,  20,   1,
	  0,   0,   4,   0,   0, 128,
	 20,   1,   0,   0,   4,   0,
	  0,   0,  40,   1,   0,   0,
	  4,   0,   0, 128,  40,   1,
	  0,   0,   4,   0,   0,   0,
	  2,   0,  46,   0,  14,   0,
	 22,   0,   2,   0,  46,   0,
	 14,   0,  22,   0,   2,   0,
	 46,   0,  13,   0,  27,   0,
	  2,   0,  46,   0,  31,   0,
	 39,   0,   2,   0,  46,   0,
	 31,   0,  39,   0,   2,   0,
	 46,   0,  31,   0,  39,   0,
	  2,   0,  46,   0,  30,   0,
	 44,   0,   2,   0,  37,   0,
	  9,   0,  36,   0,   2,   0,
	 37,   0,   2,   0,  37,   0,
	  2,   0,  37,   0,   2,   0,
	 37,   0,   2,   0,  37,   0,
	  2,   0,  37,   0, 246,   0,
	  0,   0,   4,   0,   0,   0,
	  0,   0,   0,   0,   4,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  11, 202,
	 49,   1,  56,   0,   0,   0,
	  0,  16,   0,   0,   5,  16,
	  0,   0,  92,   0,   0,   0,
	 10,   0, 255, 255,   4,   0,
	  0,   0, 255, 255,   3,   0,
	  0,   0,   0,   0,  20,   0,
	  0,   0,  20,   0,   0,   0,
	  8,   0,   0,   0,  28,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  27,  21,  64,   0,
	  0,   0,   2,   0,   0,   0,
	  8,   0, 102, 108, 111,  97,
	116,  50,   0, 243, 242, 241,
	 14,   0,   1,  18,   2,   0,
	  0,   0,   0,  16,   0,   0,
	  0,  16,   0,   0,  22,   0,
	 27,  21,  64,   0,   0,   0,
	  4,   0,   0,   0,  16,   0,
	102, 108, 111,  97, 116,  52,
	  0, 243, 242, 241,  10,   0,
	 24,  21,   2,  16,   0,   0,
	  1,   0,   1,   0,  14,   0,
	  8,  16,   3,  16,   0,   0,
	 23,   0,   2,   0,   1,  16,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 11, 202,  49,   1,  56,   0,
	  0,   0,   0,  16,   0,   0,
	  0,  16,   0,   0,   0,   0,
	  0,   0,  11,   0, 255, 255,
	  4,   0,   0,   0, 255, 255,
	  3,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	 26,   9,  47, 241,   8,   0,
	  0,   0,   8,   2,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  32,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 18,   0,  37,  17,   0,   0,
	  0,   0, 128,   0,   0,   0,
	  1,   0, 109,  97, 105, 110,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 255, 255,
	255, 255,  26,   9,  47, 241,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  16,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255,  26,   9,
	 47, 241,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 255, 255,
	255, 255, 119,   9,  49,   1,
	  1,   0,   0,   0,  13,   0,
	  0, 142,  14,   0,  63,  92,
	 15,   0,   0,   0,  76,   0,
	  0,   0,  32,   0,   0,   0,
	 44,   0,   0,   0,  84,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	  0,   0,  25,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,  44,   1,   0,   0,
	 32,   0,   0,  96,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,
	  9,   0,   4,   2,   0,   0,
	  0,   0,   0,   0,  84,   1,
	  0,   0,   1,   0,   0,   0,
	  8, 212,  87,   3,   0,   0,
	  0,   0,   0,   0,   0,   0,
	109,  97, 105, 110,   0, 110,
	111, 110, 101,   0,   0,   0,
	 45, 186,  46, 241,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 44,   1,   0,   0,  32,   0,
	  0,  96,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   2,   0,   2,   0,
	  7,   0,   0,   0,   0,   0,
	  1,   0, 255, 255, 255, 255,
	  0,   0,   0,   0,  44,   1,
	  0,   0,   8,   2,   0,   0,
	  0,   0,   0,   0, 255, 255,
	255, 255,   0,   0,   0,   0,
	255, 255, 255, 255,   1,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,  67,  58,
	 92,  85, 115, 101, 114, 115,
	 92, 108, 108, 101, 111, 110,
	 97, 114, 116,  92,  68, 101,
	115, 107, 116, 111, 112,  92,
	 80, 104,  68,  32,  80, 114,
	111, 106, 101,  99, 116,  92,
	 67,  65,  52,  71,  92,  83,
	104,  97, 100, 101, 114, 115,
	 92,  68, 114,  97, 119,  83,
	 99, 114, 101, 101, 110,  95,
	 86,  83,  46, 104, 108, 115,
	108,   0,   0,   0, 254, 239,
	254, 239,   1,   0,   0,   0,
	  1,   0,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255,  12,   0, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	148,  46,  49,   1, 251, 103,
	166,  92,   1,   0,   0,   0,
	 89,  73, 115,  61, 234,  36,
	100,  75, 169,  34, 138, 198,
	225, 156,  23, 109, 115,   0,
	  0,   0,  47,  76, 105, 110,
	107,  73, 110, 102, 111,   0,
	 47, 110,  97, 109, 101, 115,
	  0,  47, 115, 114,  99,  47,
	104, 101,  97, 100, 101, 114,
	 98, 108, 111,  99, 107,   0,
	 47, 115, 114,  99,  47, 102,
	105, 108, 101, 115,  47,  99,
	 58,  92, 117, 115, 101, 114,
	115,  92, 108, 108, 101, 111,
	110,  97, 114, 116,  92, 100,
	101, 115, 107, 116, 111, 112,
	 92, 112, 104, 100,  32, 112,
	114, 111, 106, 101,  99, 116,
	 92,  99,  97,  52, 103,  92,
	115, 104,  97, 100, 101, 114,
	115,  92, 100, 114,  97, 119,
	115,  99, 114, 101, 101, 110,
	 95, 118, 115,  46, 104, 108,
	115, 108,   0,   4,   0,   0,
	  0,   6,   0,   0,   0,   1,
	  0,   0,   0,  58,   0,   0,
	  0,   0,   0,   0,   0,  17,
	  0,   0,   0,   7,   0,   0,
	  0,  10,   0,   0,   0,   6,
	  0,   0,   0,   0,   0,   0,
	  0,   5,   0,   0,   0,  34,
	  0,   0,   0,   8,   0,   0,
	  0,   0,   0,   0,   0, 220,
	 81,  51,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  16,   0,   0,   0,
	 32,   0,   0,   0, 207,   0,
	  0,   0, 148,   0,   0,   0,
	 91,   1,   0,   0,  56,   0,
	  0,   0,   0,   0,   0,   0,
	101,   1,   0,   0, 128,   0,
	  0,   0, 166,   0,   0,   0,
	 96,   3,   0,   0,  28,   0,
	  0,   0,   0,   0,   0,   0,
	 40,   0,   0,   0,  32,   2,
	  0,   0,  44,   0,   0,   0,
	 20,   0,   0,   0,   3,   0,
	  0,   0,  20,   0,   0,   0,
	 13,   0,   0,   0,  19,   0,
	  0,   0,  14,   0,   0,   0,
	  9,   0,   0,   0,  10,   0,
	  0,   0,   8,   0,   0,   0,
	 11,   0,   0,   0,  12,   0,
	  0,   0,   7,   0,   0,   0,
	  6,   0,   0,   0,  15,   0,
	  0,   0,  16,   0,   0,   0,
	 18,   0,   0,   0,  17,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  21,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0
};
#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// tex                               texture    sint          2d             t0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float       
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_resource_texture2d(sint, sint, sint, sint) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 12
dcl_indexableTemp x0[11], 4
//
// Initial variable locations:
//   v0.x <- proj.x; v0.y <- proj.y; v0.z <- proj.z; v0.w <- proj.w; 
//   v1.x <- C.x; v1.y <- C.y; 
//   o0.x <- <main return value>.x; o0.y <- <main return value>.y; o0.z <- <main return value>.z; o0.w <- <main return value>.w
//
#line 34 "C:\Users\lleonart\Desktop\PhD Project\CA4G\Shaders\DrawComplexity_PS.hlsl"
resinfo_indexable(texture2d)(sint, sint, sint, sint) r0.xy, l(0), t0.xyzw
ftoi r0.x, r0.x  // r0.x <- width
ftoi r0.y, r0.y  // r0.y <- height

#line 35
nop
itof r0.x, r0.x
mul r0.x, r0.x, v1.x
itof r0.y, r0.y
mul r0.y, r0.y, v1.y
ftou r1.x, r0.x
ftou r1.y, r0.y
mov r1.zw, l(0, 0, 0, 0)
ld_indexable(texture2d)(sint, sint, sint, sint) r0.x, r1.xyzw, t0.xyzw
mov r0.x, r0.x

#line 5
ieq r0.y, r0.x, l(0)
if_z r0.x

#line 6
itof r1.xyz, l(1, 1, 1, 0)
endif   // r1.x <- <GetColor return value>.x; r1.y <- <GetColor return value>.y; r1.z <- <GetColor return value>.z

#line 10
if_z r0.y
itof r0.x, r0.x
log r0.x, r0.x  // r0.x <- level

#line 11
itof r2.xy, l(0, 0, 0, 0)
itof r0.yzw, l(0, 0, 0, 1)
itof r3.xz, l(0, 0, 1, 0)
itof r4.xyz, l(0, 1, 1, 0)
itof r5.xy, l(0, 1, 0, 0)
itof r6.xyz, l(0, 1, 0, 0)
itof r7.yz, l(0, 1, 0, 0)
itof r8.xyz, l(1, 1, 0, 0)
itof r9.xz, l(1, 0, 0, 0)
itof r10.xyz, l(1, 0, 0, 0)
itof r11.xyz, l(1, 0, 1, 0)
mov r2.z, l(0.500000)
mov x0[0].xyz, r2.xyzx
mov x0[1].xyz, r0.yzwy
mov r3.y, l(0.500000)
mov x0[2].xyz, r3.xyzx
mov x0[3].xyz, r4.xyzx
mov r5.z, l(0.500000)
mov x0[4].xyz, r5.xyzx
mov x0[5].xyz, r6.xyzx
mov r7.x, l(0.500000)
mov x0[6].xyz, r7.xyzx
mov x0[7].xyz, r8.xyzx
mov r9.y, l(0.500000)
mov x0[8].xyz, r9.xyzx
mov x0[9].xyz, r10.xyzx
mov x0[10].xyz, r11.xyzx

#line 25
itof r0.y, l(10)
ge r0.y, r0.x, r0.y
if_nz r0.y

#line 26
mov r1.xyz, x0[10].xyzx
endif

#line 28
if_z r0.y
ftoi r0.y, r0.x
mov r0.yzw, x0[r0.y + 0].xxyz
ftoi r1.w, r0.x
iadd r1.w, r1.w, l(1)
mov r2.xyz, x0[r1.w + 0].xyzx
itof r1.w, l(1)
mul r2.w, r0.x, r1.w
mov r3.x, -r2.w
ge r2.w, r2.w, r3.x
mov r3.x, -r1.w
movc r1.w, r2.w, r1.w, r3.x
div r2.w, l(1.000000, 1.000000, 1.000000, 1.000000), r1.w
mul r0.x, r0.x, r2.w
frc r0.x, r0.x
mul r0.x, r0.x, r1.w
mov r3.xyz, -r0.yzwy
add r2.xyz, r2.xyzx, r3.xyzx
mul r2.xyz, r0.xxxx, r2.xyzx
add r1.xyz, r0.yzwy, r2.xyzx
endif
endif

#line 35
itof o0.w, l(1)
mov o0.xyz, r1.xyzx
ret
// Approximately 77 instruction slots used
#endif

const BYTE cso_DrawComplexity_PS[] =
{
	 68,  88,  66,  67, 179, 179,
	 58, 164, 184,  29, 196,  29,
	 46,   4, 170,  29, 129, 179,
	  9, 167,   1,   0,   0,   0,
	 92,  79,   0,   0,   6,   0,
	  0,   0,  56,   0,   0,   0,
	200,   0,   0,   0,  32,   1,
	  0,   0,  84,   1,   0,   0,
	184,   8,   0,   0,  84,   9,
	  0,   0,  82,  68,  69,  70,
	136,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,  60,   0,
	  0,   0,   0,   5, 255, 255,
	  5,   1,   0,   0,  96,   0,
	  0,   0,  82,  68,  49,  49,
	 60,   0,   0,   0,  24,   0,
	  0,   0,  32,   0,   0,   0,
	 40,   0,   0,   0,  36,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   0,   0,   0,  92,   0,
	  0,   0,   2,   0,   0,   0,
	  3,   0,   0,   0,   4,   0,
	  0,   0, 255, 255, 255, 255,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	116, 101, 120,   0,  77, 105,
	 99, 114, 111, 115, 111, 102,
	116,  32,  40,  82,  41,  32,
	 72,  76,  83,  76,  32,  83,
	104,  97, 100, 101, 114,  32,
	 67, 111, 109, 112, 105, 108,
	101, 114,  32,  49,  48,  46,
	 49,   0,  73,  83,  71,  78,
	 80,   0,   0,   0,   2,   0,
	  0,   0,   8,   0,   0,   0,
	 56,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,  15,   0,   0,   0,
	 68,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   1,   0,
	  0,   0,   3,   3,   0,   0,
	 83,  86,  95,  80,  79,  83,
	 73,  84,  73,  79,  78,   0,
	 84,  69,  88,  67,  79,  79,
	 82,  68,   0, 171, 171, 171,
	 79,  83,  71,  78,  44,   0,
	  0,   0,   1,   0,   0,   0,
	  8,   0,   0,   0,  32,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	 15,   0,   0,   0,  83,  86,
	 95,  84,  65,  82,  71,  69,
	 84,   0, 171, 171,  83,  72,
	 69,  88,  92,   7,   0,   0,
	 80,   0,   0,   0, 215,   1,
	  0,   0, 106, 136,   0,   1,
	 88,  24,   0,   4,   0, 112,
	 16,   0,   0,   0,   0,   0,
	 51,  51,   0,   0,  98,  16,
	  0,   3,  50,  16,  16,   0,
	  1,   0,   0,   0, 101,   0,
	  0,   3, 242,  32,  16,   0,
	  0,   0,   0,   0, 104,   0,
	  0,   2,  12,   0,   0,   0,
	105,   0,   0,   4,   0,   0,
	  0,   0,  11,   0,   0,   0,
	  4,   0,   0,   0,  61,   0,
	  0, 137, 194,   0,   0, 128,
	195, 204,  12,   0,  50,   0,
	 16,   0,   0,   0,   0,   0,
	  1,  64,   0,   0,   0,   0,
	  0,   0,  70, 126,  16,   0,
	  0,   0,   0,   0,  27,   0,
	  0,   5,  18,   0,  16,   0,
	  0,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 27,   0,   0,   5,  34,   0,
	 16,   0,   0,   0,   0,   0,
	 26,   0,  16,   0,   0,   0,
	  0,   0,  58,   0,   0,   1,
	 43,   0,   0,   5,  18,   0,
	 16,   0,   0,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,  56,   0,   0,   7,
	 18,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  10,  16,
	 16,   0,   1,   0,   0,   0,
	 43,   0,   0,   5,  34,   0,
	 16,   0,   0,   0,   0,   0,
	 26,   0,  16,   0,   0,   0,
	  0,   0,  56,   0,   0,   7,
	 34,   0,  16,   0,   0,   0,
	  0,   0,  26,   0,  16,   0,
	  0,   0,   0,   0,  26,  16,
	 16,   0,   1,   0,   0,   0,
	 28,   0,   0,   5,  18,   0,
	 16,   0,   1,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,  28,   0,   0,   5,
	 34,   0,  16,   0,   1,   0,
	  0,   0,  26,   0,  16,   0,
	  0,   0,   0,   0,  54,   0,
	  0,   8, 194,   0,  16,   0,
	  1,   0,   0,   0,   2,  64,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 45,   0,   0, 137, 194,   0,
	  0, 128, 195, 204,  12,   0,
	 18,   0,  16,   0,   0,   0,
	  0,   0,  70,  14,  16,   0,
	  1,   0,   0,   0,  70, 126,
	 16,   0,   0,   0,   0,   0,
	 54,   0,   0,   5,  18,   0,
	 16,   0,   0,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,  32,   0,   0,   7,
	 34,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,   1,  64,
	  0,   0,   0,   0,   0,   0,
	 31,   0,   0,   3,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 43,   0,   0,   8, 114,   0,
	 16,   0,   1,   0,   0,   0,
	  2,  64,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,  21,   0,   0,   1,
	 31,   0,   0,   3,  26,   0,
	 16,   0,   0,   0,   0,   0,
	 43,   0,   0,   5,  18,   0,
	 16,   0,   0,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,  47,   0,   0,   5,
	 18,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  43,   0,
	  0,   8,  50,   0,  16,   0,
	  2,   0,   0,   0,   2,  64,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 43,   0,   0,   8, 226,   0,
	 16,   0,   0,   0,   0,   0,
	  2,  64,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,  43,   0,   0,   8,
	 82,   0,  16,   0,   3,   0,
	  0,   0,   2,  64,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,  43,   0,
	  0,   8, 114,   0,  16,   0,
	  4,   0,   0,   0,   2,  64,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 43,   0,   0,   8,  50,   0,
	 16,   0,   5,   0,   0,   0,
	  2,  64,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  43,   0,   0,   8,
	114,   0,  16,   0,   6,   0,
	  0,   0,   2,  64,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  43,   0,
	  0,   8,  98,   0,  16,   0,
	  7,   0,   0,   0,   2,  64,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 43,   0,   0,   8, 114,   0,
	 16,   0,   8,   0,   0,   0,
	  2,  64,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  43,   0,   0,   8,
	 82,   0,  16,   0,   9,   0,
	  0,   0,   2,  64,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  43,   0,
	  0,   8, 114,   0,  16,   0,
	 10,   0,   0,   0,   2,  64,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 43,   0,   0,   8, 114,   0,
	 16,   0,  11,   0,   0,   0,
	  2,  64,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,  54,   0,   0,   5,
	 66,   0,  16,   0,   2,   0,
	  0,   0,   1,  64,   0,   0,
	  0,   0,   0,  63,  54,   0,
	  0,   6, 114,  48,  32,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  70,   2,  16,   0,
	  2,   0,   0,   0,  54,   0,
	  0,   6, 114,  48,  32,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0, 150,   7,  16,   0,
	  0,   0,   0,   0,  54,   0,
	  0,   5,  34,   0,  16,   0,
	  3,   0,   0,   0,   1,  64,
	  0,   0,   0,   0,   0,  63,
	 54,   0,   0,   6, 114,  48,
	 32,   0,   0,   0,   0,   0,
	  2,   0,   0,   0,  70,   2,
	 16,   0,   3,   0,   0,   0,
	 54,   0,   0,   6, 114,  48,
	 32,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,  70,   2,
	 16,   0,   4,   0,   0,   0,
	 54,   0,   0,   5,  66,   0,
	 16,   0,   5,   0,   0,   0,
	  1,  64,   0,   0,   0,   0,
	  0,  63,  54,   0,   0,   6,
	114,  48,  32,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	 70,   2,  16,   0,   5,   0,
	  0,   0,  54,   0,   0,   6,
	114,  48,  32,   0,   0,   0,
	  0,   0,   5,   0,   0,   0,
	 70,   2,  16,   0,   6,   0,
	  0,   0,  54,   0,   0,   5,
	 18,   0,  16,   0,   7,   0,
	  0,   0,   1,  64,   0,   0,
	  0,   0,   0,  63,  54,   0,
	  0,   6, 114,  48,  32,   0,
	  0,   0,   0,   0,   6,   0,
	  0,   0,  70,   2,  16,   0,
	  7,   0,   0,   0,  54,   0,
	  0,   6, 114,  48,  32,   0,
	  0,   0,   0,   0,   7,   0,
	  0,   0,  70,   2,  16,   0,
	  8,   0,   0,   0,  54,   0,
	  0,   5,  34,   0,  16,   0,
	  9,   0,   0,   0,   1,  64,
	  0,   0,   0,   0,   0,  63,
	 54,   0,   0,   6, 114,  48,
	 32,   0,   0,   0,   0,   0,
	  8,   0,   0,   0,  70,   2,
	 16,   0,   9,   0,   0,   0,
	 54,   0,   0,   6, 114,  48,
	 32,   0,   0,   0,   0,   0,
	  9,   0,   0,   0,  70,   2,
	 16,   0,  10,   0,   0,   0,
	 54,   0,   0,   6, 114,  48,
	 32,   0,   0,   0,   0,   0,
	 10,   0,   0,   0,  70,   2,
	 16,   0,  11,   0,   0,   0,
	 43,   0,   0,   5,  34,   0,
	 16,   0,   0,   0,   0,   0,
	  1,  64,   0,   0,  10,   0,
	  0,   0,  29,   0,   0,   7,
	 34,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  26,   0,
	 16,   0,   0,   0,   0,   0,
	 31,   0,   4,   3,  26,   0,
	 16,   0,   0,   0,   0,   0,
	 54,   0,   0,   6, 114,   0,
	 16,   0,   1,   0,   0,   0,
	 70,  50,  32,   0,   0,   0,
	  0,   0,  10,   0,   0,   0,
	 21,   0,   0,   1,  31,   0,
	  0,   3,  26,   0,  16,   0,
	  0,   0,   0,   0,  27,   0,
	  0,   5,  34,   0,  16,   0,
	  0,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 54,   0,   0,   7, 226,   0,
	 16,   0,   0,   0,   0,   0,
	  6,  57,  32,   4,   0,   0,
	  0,   0,  26,   0,  16,   0,
	  0,   0,   0,   0,  27,   0,
	  0,   5, 130,   0,  16,   0,
	  1,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 30,   0,   0,   7, 130,   0,
	 16,   0,   1,   0,   0,   0,
	 58,   0,  16,   0,   1,   0,
	  0,   0,   1,  64,   0,   0,
	  1,   0,   0,   0,  54,   0,
	  0,   7, 114,   0,  16,   0,
	  2,   0,   0,   0,  70,  50,
	 32,   4,   0,   0,   0,   0,
	 58,   0,  16,   0,   1,   0,
	  0,   0,  43,   0,   0,   5,
	130,   0,  16,   0,   1,   0,
	  0,   0,   1,  64,   0,   0,
	  1,   0,   0,   0,  56,   0,
	  0,   7, 130,   0,  16,   0,
	  2,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 58,   0,  16,   0,   1,   0,
	  0,   0,  54,   0,   0,   6,
	 18,   0,  16,   0,   3,   0,
	  0,   0,  58,   0,  16, 128,
	 65,   0,   0,   0,   2,   0,
	  0,   0,  29,   0,   0,   7,
	130,   0,  16,   0,   2,   0,
	  0,   0,  58,   0,  16,   0,
	  2,   0,   0,   0,  10,   0,
	 16,   0,   3,   0,   0,   0,
	 54,   0,   0,   6,  18,   0,
	 16,   0,   3,   0,   0,   0,
	 58,   0,  16, 128,  65,   0,
	  0,   0,   1,   0,   0,   0,
	 55,   0,   0,   9, 130,   0,
	 16,   0,   1,   0,   0,   0,
	 58,   0,  16,   0,   2,   0,
	  0,   0,  58,   0,  16,   0,
	  1,   0,   0,   0,  10,   0,
	 16,   0,   3,   0,   0,   0,
	 14,   0,   0,  10, 130,   0,
	 16,   0,   2,   0,   0,   0,
	  2,  64,   0,   0,   0,   0,
	128,  63,   0,   0, 128,  63,
	  0,   0, 128,  63,   0,   0,
	128,  63,  58,   0,  16,   0,
	  1,   0,   0,   0,  56,   0,
	  0,   7,  18,   0,  16,   0,
	  0,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 58,   0,  16,   0,   2,   0,
	  0,   0,  26,   0,   0,   5,
	 18,   0,  16,   0,   0,   0,
	  0,   0,  10,   0,  16,   0,
	  0,   0,   0,   0,  56,   0,
	  0,   7,  18,   0,  16,   0,
	  0,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 58,   0,  16,   0,   1,   0,
	  0,   0,  54,   0,   0,   6,
	114,   0,  16,   0,   3,   0,
	  0,   0, 150,   7,  16, 128,
	 65,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   7,
	114,   0,  16,   0,   2,   0,
	  0,   0,  70,   2,  16,   0,
	  2,   0,   0,   0,  70,   2,
	 16,   0,   3,   0,   0,   0,
	 56,   0,   0,   7, 114,   0,
	 16,   0,   2,   0,   0,   0,
	  6,   0,  16,   0,   0,   0,
	  0,   0,  70,   2,  16,   0,
	  2,   0,   0,   0,   0,   0,
	  0,   7, 114,   0,  16,   0,
	  1,   0,   0,   0, 150,   7,
	 16,   0,   0,   0,   0,   0,
	 70,   2,  16,   0,   2,   0,
	  0,   0,  21,   0,   0,   1,
	 21,   0,   0,   1,  43,   0,
	  0,   5, 130,  32,  16,   0,
	  0,   0,   0,   0,   1,  64,
	  0,   0,   1,   0,   0,   0,
	 54,   0,   0,   5, 114,  32,
	 16,   0,   0,   0,   0,   0,
	 70,   2,  16,   0,   1,   0,
	  0,   0,  62,   0,   0,   1,
	 83,  84,  65,  84, 148,   0,
	  0,   0,  77,   0,   0,   0,
	 12,   0,   0,   0,   0,   0,
	  0,   0,   2,   0,   0,   0,
	 16,   0,   0,   0,   2,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   4,   0,
	  0,   0,   0,   0,   0,   0,
	 11,   0,   0,   0,  14,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  8,   0,   0,   0,   1,   0,
	  0,   0,  25,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 83,  80,  68,  66,   0,  70,
	  0,   0,  77, 105,  99, 114,
	111, 115, 111, 102, 116,  32,
	 67,  47,  67,  43,  43,  32,
	 77,  83,  70,  32,  55,  46,
	 48,  48,  13,  10,  26,  68,
	 83,   0,   0,   0,   0,   2,
	  0,   0,   2,   0,   0,   0,
	 35,   0,   0,   0, 168,   0,
	  0,   0,   0,   0,   0,   0,
	 31,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 192, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	 56,   0,   0,   0, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255,   5,   0,   0,   0,
	 32,   0,   0,   0,  60,   0,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255,   0,   0,
	  0,   0,   6,   0,   0,   0,
	  5,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	148,  46,  49,   1, 250, 103,
	166,  92,   1,   0,   0,   0,
	187,  24, 125, 247, 150, 195,
	117,  70, 168, 206,  99, 144,
	125,  61,  40, 142,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 220,  81,  51,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  11, 202,  49,   1,
	 56,   0,   0,   0,   0,  16,
	  0,   0,   1,  16,   0,   0,
	 24,   0,   0,   0,  11,   0,
	255, 255,   4,   0,   0,   0,
	255, 255,   3,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	  4,   0,   0,   0,   8,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	  1,  22,   0,   0,   0,   0,
	  8,  16,   0,   0,  71, 101,
	116,  67, 111, 108, 111, 114,
	  0, 243, 242, 241,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 198,  90,
	  0,   0, 117, 131,   1,   0,
	178, 211,   1,   0,  65,  36,
	  1,   0, 137, 231,   1,   0,
	202,  10,   1,   0,  76, 232,
	  3,   0,  49, 251,   3,   0,
	176,  22,   1,   0, 240, 220,
	  1,   0, 104, 174,   1,   0,
	 73,  20,   1,   0, 153, 189,
	  3,   0,   0,  16,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 84, 101, 120, 116, 117, 114,
	101,  50,  68,  60, 105, 110,
	116,  62,  32, 116, 101, 120,
	 32,  58,  32, 114, 101, 103,
	105, 115, 116, 101, 114,  32,
	 40, 116,  48,  41,  59,  13,
	 10,  13,  10, 102, 108, 111,
	 97, 116,  51,  32,  71, 101,
	116,  67, 111, 108, 111, 114,
	 40, 105, 110, 116,  32,  99,
	111, 109, 112, 108, 101, 120,
	105, 116, 121,  41,  32, 123,
	 13,  10,  13,  10,   9, 105,
	102,  32,  40,  99, 111, 109,
	112, 108, 101, 120, 105, 116,
	121,  32,  61,  61,  32,  48,
	 41,  13,  10,   9,   9, 114,
	101, 116, 117, 114, 110,  32,
	102, 108, 111,  97, 116,  51,
	 40,  49,  44,  32,  49,  44,
	 32,  49,  41,  59,  13,  10,
	 13,  10,   9,  47,  47, 114,
	101, 116, 117, 114, 110,  32,
	102, 108, 111,  97, 116,  51,
	 40,  49,  44,  49,  44,  49,
	 41,  59,  13,  10,  13,  10,
	  9, 102, 108, 111,  97, 116,
	 32, 108, 101, 118, 101, 108,
	 32,  61,  32, 108, 111, 103,
	 50,  40,  99, 111, 109, 112,
	108, 101, 120, 105, 116, 121,
	 41,  59,  13,  10,   9, 102,
	108, 111,  97, 116,  51,  32,
	115, 116, 111, 112,  80, 111,
	105, 110, 116, 115,  91,  49,
	 49,  93,  32,  61,  32, 123,
	 13,  10,   9,   9, 102, 108,
	111,  97, 116,  51,  40,  48,
	 44,  48,  44,  48,  46,  53,
	 41,  44,  32,  47,  47,  32,
	 49,  13,  10,   9,   9, 102,
	108, 111,  97, 116,  51,  40,
	 48,  44,  48,  44,  49,  41,
	 44,  32,  47,  47,  32,  50,
	 13,  10,   9,   9, 102, 108,
	111,  97, 116,  51,  40,  48,
	 44,  48,  46,  53,  44,  49,
	 41,  44,  32,  47,  47,  32,
	 52,  13,  10,   9,   9, 102,
	108, 111,  97, 116,  51,  40,
	 48,  44,  49,  44,  49,  41,
	 44,  32,  47,  47,  32,  56,
	 13,  10,   9,   9, 102, 108,
	111,  97, 116,  51,  40,  48,
	 44,  49,  44,  48,  46,  53,
	 41,  44,  32,  47,  47,  32,
	 49,  54,  13,  10,   9,   9,
	102, 108, 111,  97, 116,  51,
	 40,  48,  44,  49,  44,  48,
	 41,  44,  32,  47,  47,  32,
	 51,  50,  13,  10,   9,   9,
	102, 108, 111,  97, 116,  51,
	 40,  48,  46,  53,  44,  49,
	 44,  48,  41,  44,  32,  47,
	 47,  32,  54,  52,  13,  10,
	  9,   9, 102, 108, 111,  97,
	116,  51,  40,  49,  44,  49,
	 44,  48,  41,  44,  32,  47,
	 47,  32,  49,  50,  56,  13,
	 10,   9,   9, 102, 108, 111,
	 97, 116,  51,  40,  49,  44,
	 48,  46,  53,  44,  48,  41,
	 44,  32,  47,  47,  32,  50,
	 53,  54,  13,  10,   9,   9,
	102, 108, 111,  97, 116,  51,
	 40,  49,  44,  48,  44,  48,
	 41,  44,  32,  47,  47,  32,
	 53,  49,  50,  13,  10,   9,
	  9, 102, 108, 111,  97, 116,
	 51,  40,  49,  44,  48,  44,
	 49,  41,  32,  47,  47,  32,
	 49,  48,  50,  52,  13,  10,
	  9, 125,  59,  13,  10,  13,
	 10,   9, 105, 102,  32,  40,
	108, 101, 118, 101, 108,  32,
	 62,  61,  32,  49,  48,  41,
	 13,  10,   9,   9, 114, 101,
	116, 117, 114, 110,  32, 115,
	116, 111, 112,  80, 111, 105,
	110, 116, 115,  91,  49,  48,
	 93,  59,  13,  10,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 108, 101, 114, 112,
	 40, 115, 116, 111, 112,  80,
	111, 105, 110, 116, 115,  91,
	 40, 105, 110, 116,  41, 108,
	101, 118, 101, 108,  93,  44,
	 32, 115, 116, 111, 112,  80,
	111, 105, 110, 116, 115,  91,
	 40, 105, 110, 116,  41, 108,
	101, 118, 101, 108,  32,  43,
	 32,  49,  93,  44,  32, 108,
	101, 118, 101, 108,  32,  37,
	 32,  49,  41,  59,  13,  10,
	125,  13,  10,  13,  10, 102,
	108, 111,  97, 116,  52,  32,
	109,  97, 105, 110,  40, 102,
	108, 111,  97, 116,  52,  32,
	112, 114, 111, 106,  32,  58,
	 32,  83,  86,  95,  80,  79,
	 83,  73,  84,  73,  79,  78,
	 44,  32, 102, 108, 111,  97,
	116,  50,  32,  67,  32,  58,
	 32,  84,  69,  88,  67,  79,
	 79,  82,  68,  41,  32,  58,
	 32,  83,  86,  95,  84,  65,
	 82,  71,  69,  84,  13,  10,
	123,  13,  10,   9, 105, 110,
	116,  32, 119, 105, 100, 116,
	104,  44,  32, 104, 101, 105,
	103, 104, 116,  59,  13,  10,
	  9, 116, 101, 120,  46,  71,
	101, 116,  68, 105, 109, 101,
	110, 115, 105, 111, 110, 115,
	 40, 119, 105, 100, 116, 104,
	 44,  32, 104, 101, 105, 103,
	104, 116,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 102, 108, 111,  97,
	116,  52,  40,  71, 101, 116,
	 67, 111, 108, 111, 114,  40,
	116, 101, 120,  91, 117, 105,
	110, 116,  50,  40, 119, 105,
	100, 116, 104,  42,  67,  46,
	120,  44,  32, 104, 101, 105,
	103, 104, 116,  42,  67,  46,
	121,  41,  93,  41,  44,  49,
	 41,  59,  13,  10, 125,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 254, 239,
	254, 239,   1,   0,   0,   0,
	210,   3,   0,   0,   0,  67,
	 58,  92,  85, 115, 101, 114,
	115,  92, 108, 108, 101, 111,
	110,  97, 114, 116,  92,  68,
	101, 115, 107, 116, 111, 112,
	 92,  80, 104,  68,  32,  80,
	114, 111, 106, 101,  99, 116,
	 92,  67,  65,  52,  71,  92,
	 83, 104,  97, 100, 101, 114,
	115,  92,  68, 114,  97, 119,
	 67, 111, 109, 112, 108, 101,
	120, 105, 116, 121,  95,  80,
	 83,  46, 104, 108, 115, 108,
	  0,   0,  99,  58,  92, 117,
	115, 101, 114, 115,  92, 108,
	108, 101, 111, 110,  97, 114,
	116,  92, 100, 101, 115, 107,
	116, 111, 112,  92, 112, 104,
	100,  32, 112, 114, 111, 106,
	101,  99, 116,  92,  99,  97,
	 52, 103,  92, 115, 104,  97,
	100, 101, 114, 115,  92, 100,
	114,  97, 119,  99, 111, 109,
	112, 108, 101, 120, 105, 116,
	121,  95, 112, 115,  46, 104,
	108, 115, 108,   0,  84, 101,
	120, 116, 117, 114, 101,  50,
	 68,  60, 105, 110, 116,  62,
	 32, 116, 101, 120,  32,  58,
	 32, 114, 101, 103, 105, 115,
	116, 101, 114,  32,  40, 116,
	 48,  41,  59,  13,  10,  13,
	 10, 102, 108, 111,  97, 116,
	 51,  32,  71, 101, 116,  67,
	111, 108, 111, 114,  40, 105,
	110, 116,  32,  99, 111, 109,
	112, 108, 101, 120, 105, 116,
	121,  41,  32, 123,  13,  10,
	 13,  10,   9, 105, 102,  32,
	 40,  99, 111, 109, 112, 108,
	101, 120, 105, 116, 121,  32,
	 61,  61,  32,  48,  41,  13,
	 10,   9,   9, 114, 101, 116,
	117, 114, 110,  32, 102, 108,
	111,  97, 116,  51,  40,  49,
	 44,  32,  49,  44,  32,  49,
	 41,  59,  13,  10,  13,  10,
	  9,  47,  47, 114, 101, 116,
	117, 114, 110,  32, 102, 108,
	111,  97, 116,  51,  40,  49,
	 44,  49,  44,  49,  41,  59,
	 13,  10,  13,  10,   9, 102,
	108, 111,  97, 116,  32, 108,
	101, 118, 101, 108,  32,  61,
	 32, 108, 111, 103,  50,  40,
	 99, 111, 109, 112, 108, 101,
	120, 105, 116, 121,  41,  59,
	 13,  10,   9, 102, 108, 111,
	 97, 116,  51,  32, 115, 116,
	111, 112,  80, 111, 105, 110,
	116, 115,  91,  49,  49,  93,
	 32,  61,  32, 123,  13,  10,
	  9,   9, 102, 108, 111,  97,
	116,  51,  40,  48,  44,  48,
	 44,  48,  46,  53,  41,  44,
	 32,  47,  47,  32,  49,  13,
	 10,   9,   9, 102, 108, 111,
	 97, 116,  51,  40,  48,  44,
	 48,  44,  49,  41,  44,  32,
	 47,  47,  32,  50,  13,  10,
	  9,   9, 102, 108, 111,  97,
	116,  51,  40,  48,  44,  48,
	 46,  53,  44,  49,  41,  44,
	 32,  47,  47,  32,  52,  13,
	 10,   9,   9, 102, 108, 111,
	 97, 116,  51,  40,  48,  44,
	 49,  44,  49,  41,  44,  32,
	 47,  47,  32,  56,  13,  10,
	  9,   9, 102, 108, 111,  97,
	116,  51,  40,  48,  44,  49,
	 44,  48,  46,  53,  41,  44,
	 32,  47,  47,  32,  49,  54,
	 13,  10,   9,   9, 102, 108,
	111,  97, 116,  51,  40,  48,
	 27, 226,  48,   1, 128,   0,
	  0,   0, 185,  96,  92, 102,
	 36, 235, 212,   1,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   2,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 76,   0,   0,   0,  40,   0,
	  0,   0,  27, 226,  48,   1,
	 82, 214, 153,   9,  59,   3,
	  0,   0,   1,   0,   0,   0,
	 75,   0,   0,   0,  76,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	 66,   0,  60,  17,  16,   1,
	  0,   0,   0,   1,  10,   0,
	  1,   0, 132,   0,  99,  69,
	 10,   0,   1,   0, 132,   0,
	 99,  69,  77, 105,  99, 114,
	111, 115, 111, 102, 116,  32,
	 40,  82,  41,  32,  72,  76,
	 83,  76,  32,  83, 104,  97,
	100, 101, 114,  32,  67, 111,
	109, 112, 105, 108, 101, 114,
	 32,  49,  48,  46,  49,   0,
	  0,   0,  54,   0,  61,  17,
	  1, 104, 108, 115, 108,  70,
	108,  97, 103, 115,   0,  48,
	120,  53,   0, 104, 108, 115,
	108,  84,  97, 114, 103, 101,
	116,   0, 112, 115,  95,  53,
	 95,  48,   0, 104, 108, 115,
	108,  69, 110, 116, 114, 121,
	  0, 109,  97, 105, 110,   0,
	  0,   0,   0,   0,  42,   0,
	 16,  17,   0,   0,   0,   0,
	 80,   5,   0,   0,   0,   0,
	  0,   0,  16,   7,   0,   0,
	  0,   0,   0,   0,  16,   7,
	  0,   0,   4,  16,   0,   0,
	 76,   0,   0,   0,   1,   0,
	160, 109,  97, 105, 110,   0,
	 42,   0,  62,  17,   0,  16,
	  0,   0,   9,   0, 112, 114,
	111, 106,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   0,   0,
	  4,   0,  76,   0,   0,   0,
	  1,   0,  16,   7,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   4,   0,
	  4,   0,  76,   0,   0,   0,
	  1,   0,  16,   7,   4,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   8,   0,
	  4,   0,  76,   0,   0,   0,
	  1,   0,  16,   7,   8,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,  12,   0,
	  4,   0,  76,   0,   0,   0,
	  1,   0,  16,   7,  12,   0,
	  0,   0,  42,   0,  62,  17,
	  1,  16,   0,   0,   9,   0,
	 67,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   1,   0,   5,   0,
	  0,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	 16,   0,   0,   0,  22,   0,
	 80,  17,   1,   0,   5,   0,
	  4,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	 20,   0,   0,   0,  58,   0,
	 62,  17,   3,  16,   0,   0,
	136,   0,  60, 109,  97, 105,
	110,  32, 114, 101, 116, 117,
	114, 110,  32, 118,  97, 108,
	117, 101,  62,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  0,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  4,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	  4,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  8,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	  8,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	 12,   0,   4,   0,  76,   0,
	  0,   0,   1,   0,  16,   7,
	 12,   0,   0,   0,  46,   0,
	 62,  17, 116,   0,   0,   0,
	  0,   0, 119, 105, 100, 116,
	104,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   0,   0,   1,   0,
	  0,   0,   4,   0, 132,   0,
	  0,   0,   1,   0,  44,   0,
	  0,   0,   0,   0,  46,   0,
	 62,  17, 116,   0,   0,   0,
	  0,   0, 104, 101, 105, 103,
	104, 116,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   0,   0,   1,   0,
	  0,   0,   4,   0, 152,   0,
	  0,   0,   1,   0,  72,   0,
	  4,   0,   0,   0,  34,   1,
	 77,  17, 128,   0,   0,   0,
	 76,   5,   0,   0,   0,  16,
	  0,   0,   7,   0,   9,   2,
	 13,  21,   6,   4,   3, 129,
	 48,   9,   3,  13,  25,   6,
	  2,   3,  40,   9,  25,   3,
	 32,   9,   2,  13,  32,   6,
	  8,   3,   4,   7,  12,  13,
	  3,   6,   2,   3,  52,   7,
	  0,  13,  17,   6,  28,   3,
	130, 204,   9,   3,  13,  24,
	  6,   2,   3,  60,   9,  24,
	  3,  24,   9,   2,  13,  76,
	 11,  68,   9,  76,  12,   8,
	130,   8,   8,   0,   9,   6,
	 13,  20,   1, 129, 124,   6,
	 47,   3,   0,   9,   2,  13,
	 21,   3,  28,   9,  10,  13,
	 24,  11,  44,   9,  25,  13,
	 25,   3,  32,   9,   2,  13,
	 32,   6,   8,   3,   4,   9,
	 16,  13,  31,   3,  12,   9,
	  3,  13,  17,   6,   4,   3,
	 40,  13,  15,   6,   2,   3,
	 32,  13,  17,   6,   2,   3,
	 32,  13,  15,   6,   2,   3,
	 32,  13,  17,   6,   2,   3,
	 32,  13,  15,   6,   2,   3,
	 32,  13,  17,   6,   2,   3,
	 32,  13,  15,   6,   2,   3,
	 32,  13,  17,   6,   2,   3,
	 32,  13,  15,   6,   2,   3,
	 32,   6,   2,   3,  32,   7,
	 12,   9,   9,  13,   2,   6,
	 23,   3,  32,   7,   0,   9,
	  6,  13,  16,   6,  28,   3,
	129, 108,   9,   2,  13,  17,
	  3,  48,   9,  10,  13,  23,
	 11,  44,   9,  24,  13,  24,
	  3,  24,   9,   2,  13,  76,
	 11,  68,   9,  25,  13,  34,
	  3,  12,   9,  14,  13,  35,
	  3,  20,   9,  49,  13,  58,
	  3,  28,  13,  62,   3,  20,
	  9,  38,  13,  63,   3,  28,
	  9,  66,  13,  74,   3,  28,
	  9,   9,  13,  75,   3, 129,
	 20,   9,  76,  13,  76,  12,
	  8, 108,  62,   0,  62,  17,
	  7,  16,   0,   0, 136,   0,
	 60,  71, 101, 116,  67, 111,
	108, 111, 114,  32, 114, 101,
	116, 117, 114, 110,  32, 118,
	 97, 108, 117, 101,  62,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   0,   0,
	  5,   0,   0,   0,   4,   0,
	200,   1,   0,   0,   1,   0,
	124,   5,  16,   0,   0,   0,
	 22,   0,  80,  17,   0,   0,
	  5,   0,   4,   0,   4,   0,
	200,   1,   0,   0,   1,   0,
	124,   5,  20,   0,   0,   0,
	 22,   0,  80,  17,   0,   0,
	  5,   0,   8,   0,   4,   0,
	200,   1,   0,   0,   1,   0,
	124,   5,  24,   0,   0,   0,
	 50,   0,  62,  17, 116,   0,
	  0,   0,   1,   0,  99, 111,
	109, 112, 108, 101, 120, 105,
	116, 121,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   0,   0,   1,   0,
	  0,   0,   4,   0, 124,   1,
	  0,   0,   1,   0, 128,   0,
	  0,   0,   0,   0,  46,   0,
	 62,  17,  64,   0,   0,   0,
	  0,   0, 108, 101, 118, 101,
	108,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   0,   0,   1,   0,
	  0,   0,   4,   0, 252,   1,
	  0,   0,   1,   0, 144,   4,
	  0,   0,   0,   0,  50,   0,
	 62,  17,   9,  16,   0,   0,
	  8,   0, 115, 116, 111, 112,
	 80, 111, 105, 110, 116, 115,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  3,   0,   1,   0,   0,   0,
	172,   0, 124,   1,   0,   0,
	  1,   0, 200,   5,   0,   0,
	  0,   0,   2,   0,  78,  17,
	  2,   0,   6,   0, 244,   0,
	  0,   0,  24,   0,   0,   0,
	  1,   0,   0,   0,  16,   1,
	207,  67,   4, 234, 230,  80,
	194, 105, 158,  89, 122,  58,
	209,  90,  26,  46,   0,   0,
	242,   0,   0,   0,  80,   7,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   1,   0,  92,   7,
	  0,   0,   0,   0,   0,   0,
	154,   0,   0,   0,  68,   7,
	  0,   0,  76,   0,   0,   0,
	 34,   0,   0, 128,  76,   0,
	  0,   0,  34,   0,   0,   0,
	112,   0,   0,   0,  34,   0,
	  0, 128, 112,   0,   0,   0,
	 34,   0,   0,   0, 132,   0,
	  0,   0,  34,   0,   0, 128,
	132,   0,   0,   0,  34,   0,
	  0,   0, 152,   0,   0,   0,
	 35,   0,   0, 128, 152,   0,
	  0,   0,  35,   0,   0,   0,
	156,   0,   0,   0,  35,   0,
	  0, 128, 156,   0,   0,   0,
	 35,   0,   0,   0, 176,   0,
	  0,   0,  35,   0,   0, 128,
	176,   0,   0,   0,  35,   0,
	  0,   0, 204,   0,   0,   0,
	 35,   0,   0, 128, 204,   0,
	  0,   0,  35,   0,   0,   0,
	224,   0,   0,   0,  35,   0,
	  0, 128, 224,   0,   0,   0,
	 35,   0,   0,   0, 252,   0,
	  0,   0,  35,   0,   0, 128,
	252,   0,   0,   0,  35,   0,
	  0,   0,  16,   1,   0,   0,
	 35,   0,   0, 128,  16,   1,
	  0,   0,  35,   0,   0,   0,
	 36,   1,   0,   0,  35,   0,
	  0, 128,  36,   1,   0,   0,
	 35,   0,   0,   0,  68,   1,
	  0,   0,  35,   0,   0, 128,
	 68,   1,   0,   0,  35,   0,
	  0,   0, 104,   1,   0,   0,
	 35,   0,   0, 128, 104,   1,
	  0,   0,  35,   0,   0,   0,
	124,   1,   0,   0,  35,   0,
	  0, 128, 124,   1,   0,   0,
	 35,   0,   0,   0, 152,   1,
	  0,   0,  35,   0,   0, 128,
	152,   1,   0,   0,  35,   0,
	  0,   0, 164,   1,   0,   0,
	 35,   0,   0, 128, 164,   1,
	  0,   0,  35,   0,   0,   0,
	196,   1,   0,   0,  35,   0,
	  0, 128, 196,   1,   0,   0,
	 35,   0,   0,   0, 200,   1,
	  0,   0,  35,   0,   0, 128,
	200,   1,   0,   0,  35,   0,
	  0,   0, 212,   1,   0,   0,
	 35,   0,   0, 128, 212,   1,
	  0,   0,  35,   0,   0,   0,
	232,   1,   0,   0,  35,   0,
	  0, 128, 232,   1,   0,   0,
	 35,   0,   0,   0, 252,   1,
	  0,   0,  35,   0,   0, 128,
	252,   1,   0,   0,  35,   0,
	  0,   0,  28,   2,   0,   0,
	 35,   0,   0, 128,  28,   2,
	  0,   0,  35,   0,   0,   0,
	 60,   2,   0,   0,  35,   0,
	  0, 128,  60,   2,   0,   0,
	 35,   0,   0,   0,  92,   2,
	  0,   0,  35,   0,   0, 128,
	 92,   2,   0,   0,  35,   0,
	  0,   0, 124,   2,   0,   0,
	 35,   0,   0, 128, 124,   2,
	  0,   0,  35,   0,   0,   0,
	156,   2,   0,   0,  35,   0,
	  0, 128, 156,   2,   0,   0,
	 35,   0,   0,   0, 188,   2,
	  0,   0,  35,   0,   0, 128,
	188,   2,   0,   0,  35,   0,
	  0,   0, 220,   2,   0,   0,
	 35,   0,   0, 128, 220,   2,
	  0,   0,  35,   0,   0,   0,
	252,   2,   0,   0,  35,   0,
	  0, 128, 252,   2,   0,   0,
	 35,   0,   0,   0,  28,   3,
	  0,   0,  35,   0,   0, 128,
	 28,   3,   0,   0,  35,   0,
	  0,   0,  60,   3,   0,   0,
	 35,   0,   0, 128,  60,   3,
	  0,   0,  35,   0,   0,   0,
	 92,   3,   0,   0,  35,   0,
	  0, 128,  92,   3,   0,   0,
	 35,   0,   0,   0, 112,   3,
	  0,   0,  35,   0,   0, 128,
	112,   3,   0,   0,  35,   0,
	  0,   0, 136,   3,   0,   0,
	 35,   0,   0, 128, 136,   3,
	  0,   0,  35,   0,   0,   0,
	160,   3,   0,   0,  35,   0,
	  0, 128, 160,   3,   0,   0,
	 35,   0,   0,   0, 180,   3,
	  0,   0,  35,   0,   0, 128,
	180,   3,   0,   0,  35,   0,
	  0,   0, 204,   3,   0,   0,
	 35,   0,   0, 128, 204,   3,
	  0,   0,  35,   0,   0,   0,
	228,   3,   0,   0,  35,   0,
	  0, 128, 228,   3,   0,   0,
	 35,   0,   0,   0, 248,   3,
	  0,   0,  35,   0,   0, 128,
	248,   3,   0,   0,  35,   0,
	  0,   0,  16,   4,   0,   0,
	 35,   0,   0, 128,  16,   4,
	  0,   0,  35,   0,   0,   0,
	 40,   4,   0,   0,  35,   0,
	  0, 128,  40,   4,   0,   0,
	 35,   0,   0,   0,  60,   4,
	  0,   0,  35,   0,   0, 128,
	 60,   4,   0,   0,  35,   0,
	  0,   0,  84,   4,   0,   0,
	 35,   0,   0, 128,  84,   4,
	  0,   0,  35,   0,   0,   0,
	108,   4,   0,   0,  35,   0,
	  0, 128, 108,   4,   0,   0,
	 35,   0,   0,   0, 128,   4,
	  0,   0,  35,   0,   0, 128,
	128,   4,   0,   0,  35,   0,
	  0,   0, 152,   4,   0,   0,
	 35,   0,   0, 128, 152,   4,
	  0,   0,  35,   0,   0,   0,
	176,   4,   0,   0,  35,   0,
	  0, 128, 176,   4,   0,   0,
	 35,   0,   0,   0, 200,   4,
	  0,   0,  35,   0,   0, 128,
	200,   4,   0,   0,  35,   0,
	  0,   0, 220,   4,   0,   0,
	 35,   0,   0, 128, 220,   4,
	  0,   0,  35,   0,   0,   0,
	248,   4,   0,   0,  35,   0,
	  0, 128, 248,   4,   0,   0,
	 35,   0,   0,   0,   4,   5,
	  0,   0,  35,   0,   0, 128,
	  4,   5,   0,   0,  35,   0,
	  0,   0,  28,   5,   0,   0,
	 35,   0,   0, 128,  28,   5,
	  0,   0,  35,   0,   0,   0,
	 32,   5,   0,   0,  35,   0,
	  0, 128,  32,   5,   0,   0,
	 35,   0,   0,   0,  44,   5,
	  0,   0,  35,   0,   0, 128,
	 44,   5,   0,   0,  35,   0,
	  0,   0,  64,   5,   0,   0,
	 35,   0,   0, 128,  64,   5,
	  0,   0,  35,   0,   0,   0,
	 92,   5,   0,   0,  35,   0,
	  0, 128,  92,   5,   0,   0,
	 35,   0,   0,   0, 112,   5,
	  0,   0,  35,   0,   0, 128,
	112,   5,   0,   0,  35,   0,
	  0,   0, 140,   5,   0,   0,
	 35,   0,   0, 128, 140,   5,
	  0,   0,  35,   0,   0,   0,
	168,   5,   0,   0,  35,   0,
	  0, 128, 168,   5,   0,   0,
	 35,   0,   0,   0, 188,   5,
	  0,   0,  35,   0,   0, 128,
	188,   5,   0,   0,  35,   0,
	  0,   0, 216,   5,   0,   0,
	 35,   0,   0, 128, 216,   5,
	  0,   0,  35,   0,   0,   0,
	240,   5,   0,   0,  35,   0,
	  0, 128, 240,   5,   0,   0,
	 35,   0,   0,   0,  12,   6,
	  0,   0,  35,   0,   0, 128,
	 12,   6,   0,   0,  35,   0,
	  0,   0,  36,   6,   0,   0,
	 35,   0,   0, 128,  36,   6,
	  0,   0,  35,   0,   0,   0,
	 72,   6,   0,   0,  35,   0,
	  0, 128,  72,   6,   0,   0,
	 35,   0,   0,   0, 112,   6,
	  0,   0,  35,   0,   0, 128,
	112,   6,   0,   0,  35,   0,
	  0,   0, 140,   6,   0,   0,
	 35,   0,   0, 128, 140,   6,
	  0,   0,  35,   0,   0,   0,
	160,   6,   0,   0,  35,   0,
	  0, 128, 160,   6,   0,   0,
	 35,   0,   0,   0, 188,   6,
	  0,   0,  35,   0,   0, 128,
	188,   6,   0,   0,  35,   0,
	  0,   0, 212,   6,   0,   0,
	 35,   0,   0, 128, 212,   6,
	  0,   0,  35,   0,   0,   0,
	240,   6,   0,   0,  35,   0,
	  0, 128, 240,   6,   0,   0,
	 35,   0,   0,   0,  12,   7,
	  0,   0,  35,   0,   0, 128,
	 12,   7,   0,   0,  35,   0,
	  0,   0,  40,   7,   0,   0,
	 35,   0,   0, 128,  40,   7,
	  0,   0,  35,   0,   0,   0,
	 44,   7,   0,   0,  35,   0,
	  0, 128,  44,   7,   0,   0,
	 35,   0,   0,   0,  48,   7,
	  0,   0,  35,   0,   0, 128,
	 48,   7,   0,   0,  35,   0,
	  0,   0,  68,   7,   0,   0,
	 35,   0,   0, 128,  68,   7,
	  0,   0,  35,   0,   0,   0,
	 88,   7,   0,   0,  35,   0,
	  0, 128,  88,   7,   0,   0,
	 35,   0,   0,   0,   2,   0,
	 34,   0,   2,   0,  33,   0,
	  2,   0,  34,   0,   2,   0,
	 33,   0,   2,   0,  34,   0,
	  2,   0,  33,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  35,   0,
	 43,   0,   2,   0,  62,   0,
	 35,   0,  43,   0,   2,   0,
	 62,   0,  46,   0,  55,   0,
	  2,   0,  62,   0,  46,   0,
	 55,   0,   2,   0,  62,   0,
	 29,   0,  56,   0,   2,   0,
	 62,   0,  29,   0,  56,   0,
	  2,   0,  62,   0,  25,   0,
	 57,   0,   2,   0,  62,   0,
	 25,   0,  57,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	 16,   0,  58,   0,   2,   0,
	 62,   0,  16,   0,  58,   0,
	  2,   0,  62,   0,  16,   0,
	 58,   0,   2,   0,  62,   0,
	  9,   0,  61,   0,   2,   0,
	 62,   0,   2,   0,  62,   0,
	  2,   0,  62,   0,   2,   0,
	 62,   0, 246,   0,   0,   0,
	 16,   0,   0,   0,   0,   0,
	  0,   0,   0,  16,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   8,   0,   0,   0,
	  0,   0,   0,   0,  20,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  11, 202,
	 49,   1,  56,   0,   0,   0,
	  0,  16,   0,   0,  13,  16,
	  0,   0, 216,   0,   0,   0,
	 10,   0, 255, 255,   4,   0,
	  0,   0, 255, 255,   3,   0,
	  0,   0,   0,   0,  52,   0,
	  0,   0,  52,   0,   0,   0,
	  8,   0,   0,   0,  60,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  27,  21,  64,   0,
	  0,   0,   4,   0,   0,   0,
	 16,   0, 102, 108, 111,  97,
	116,  52,   0, 243, 242, 241,
	 22,   0,  27,  21,  64,   0,
	  0,   0,   2,   0,   0,   0,
	  8,   0, 102, 108, 111,  97,
	116,  50,   0, 243, 242, 241,
	 14,   0,   1,  18,   2,   0,
	  0,   0,   0,  16,   0,   0,
	  1,  16,   0,   0,  10,   0,
	 24,  21,   0,  16,   0,   0,
	  1,   0,   1,   0,  14,   0,
	  8,  16,   3,  16,   0,   0,
	 23,   0,   2,   0,   2,  16,
	  0,   0,  10,   0,   1,  18,
	  1,   0,   0,   0, 116,   0,
	  0,   0,  22,   0,  27,  21,
	 64,   0,   0,   0,   3,   0,
	  0,   0,  12,   0, 102, 108,
	111,  97, 116,  51,   0, 243,
	242, 241,  10,   0,  24,  21,
	  6,  16,   0,   0,   1,   0,
	  1,   0,  14,   0,   8,  16,
	  7,  16,   0,   0,  23,   0,
	  1,   0,   5,  16,   0,   0,
	 18,   0,  22,  21,   6,  16,
	  0,   0,  34,   0,   0,   0,
	 16,   0,   0,   0, 172,   0,
	  0, 241,  14,   0,  23,  21,
	116,   0,   0,   0,   3,   2,
	  0,   0,   0,   0, 242, 241,
	 10,   0,  24,  21,  10,  16,
	  0,   0,   1,   0,   1,   0,
	 10,   0,  24,  21,  11,  16,
	  0,   0,   1,   0,   0,   2,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 43,  73,   0,   0,   0,  16,
	  0,   0,   0,   0,   0,   0,
	  0,  16,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	  4,   0,   0,   0, 255, 255,
	  3,   0,   0,   0,   0,   0,
	255, 255, 255, 255,   0,   0,
	  0,   0, 255, 255, 255, 255,
	  0,   0,   0,   0, 255, 255,
	255, 255,  22,   0,  27,  21,
	 64,   0,   0,   0,   4,   0,
	  0,   0,  16,   0, 102, 108,
	111,  97, 116,  52,   0, 243,
	242, 241,  22,   0,  27,  21,
	 64,   0,   0,   0,   2,   0,
	  0,   0,   8,   0, 102, 108,
	111,  97, 116,  50,   0, 243,
	242, 241,  14,   0,   1,  18,
	  2,   0,   0,   0,   0,  16,
	  0,   0,   1,  16,   0,   0,
	 10,   0,  24,  21,   0,  16,
	  0,   0,   1,   0,   1,   0,
	 14,   0,   8,  16,   3,  16,
	  0,   0,  23,   0,   2,   0,
	  2,  16,   0,   0,  10,   0,
	  1,  18,   1,   0,   0,   0,
	116,   0,   0,   0,  22,   0,
	 27,  21,  64,   0,   0,   0,
	  3,   0,   0,   0,  12,   0,
	102, 108, 111,  97, 116,  51,
	  0, 243, 242, 241,  10,   0,
	 24,  21,   6,  16,   0,   0,
	  1,   0,   1,   0,  14,   0,
	  8,  16,   7,  16,   0,   0,
	 23,   0,   1,   0,   5,  16,
	  0,   0,  18,   0,  22,  21,
	  6,  16,   0,   0,  34,   0,
	  0,   0,  16,   0,   0,   0,
	172,   0,   0, 241,  14,   0,
	 23,  21, 116,   0,   0,   0,
	  3,   2,   0,   0,   0,   0,
	242, 241,  10,   0,  24,  21,
	 10,  16,   0,   0,   1,   0,
	  1,   0,  10,   0,  24,  21,
	 11,  16,   0,   0,   1,   0,
	  0,   2,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  68,  51,  68,  83,
	 72,  68,  82,   0,  92,   7,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  32,   0,   0,  96,
	  4,   0,   0,   0,   8,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	  1,  22,   0,   0,   0,   0,
	  8,  16,   0,   0,  71, 101,
	116,  67, 111, 108, 111, 114,
	  0, 243, 242, 241,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 255, 255,
	255, 255,  26,   9,  47, 241,
	 16,   0,   0,   0,  12,   2,
	  0,   0,  21,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  16,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  32,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  12,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  18,   0,  37,  17,
	  0,   0,   0,   0, 128,   0,
	  0,   0,   1,   0, 109,  97,
	105, 110,   0,   0,  22,   0,
	 81,  17,  12,  16,   0,   0,
	  7,   0, 255, 255, 255, 255,
	  0,   0, 255, 255, 255, 255,
	116, 101, 120,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  16,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	 26,   9,  47, 241,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255, 119,   9,
	 49,   1,   1,   0,   0,   0,
	 13,   0,   0, 142,  14,   0,
	 63,  92,  15,   0,   0,   0,
	 76,   0,   0,   0,  32,   0,
	  0,   0,  44,   0,   0,   0,
	 88,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,   0,   0,  25,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,  92,   7,
	  0,   0,  32,   0,   0,  96,
	  0,   0,  40, 142,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  2,   0,   9,   0,  84,   5,
	  0,   0,   0,   0,   0,   0,
	144,   7,   0,   0,   1,   0,
	  0,   0, 112,  26, 151,   2,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 109,  97, 105, 110,
	  0, 110, 111, 110, 101,   0,
	  0,   0,  45, 186,  46, 241,
	  1,   0,   0,   0,   0,   0,
	  0,   0,  92,   7,   0,   0,
	 32,   0,   0,  96,   0,   0,
	 40, 142,   0,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,
	  2,   0,   7,   0,   0,   0,
	  0,   0,   1,   0, 255, 255,
	255, 255,   0,   0,   0,   0,
	 92,   7,   0,   0,   8,   2,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255,   0,   0,
	  0,   0, 255, 255, 255, 255,
	  1,   0,   1,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	 67,  58,  92,  85, 115, 101,
	114, 115,  92, 108, 108, 101,
	111, 110,  97, 114, 116,  92,
	 68, 101, 115, 107, 116, 111,
	112,  92,  80, 104,  68,  32,
	 80, 114, 111, 106, 101,  99,
	116,  92,  67,  65,  52,  71,
	 92,  83, 104,  97, 100, 101,
	114, 115,  92,  68, 114,  97,
	119,  67, 111, 109, 112, 108,
	101, 120, 105, 116, 121,  95,
	 80,  83,  46, 104, 108, 115,
	108,   0,   0,   0, 254, 239,
	254, 239,   1,   0,   0,   0,
	  1,   0,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255,  12,   0, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  44,  49,  44,  48,
	 41,  44,  32,  47,  47,  32,
	 51,  50,  13,  10,   9,   9,
	102, 108, 111,  97, 116,  51,
	 40,  48,  46,  53,  44,  49,
	 44,  48,  41,  44,  32,  47,
	 47,  32,  54,  52,  13,  10,
	  9,   9, 102, 108, 111,  97,
	116,  51,  40,  49,  44,  49,
	 44,  48,  41,  44,  32,  47,
	 47,  32,  49,  50,  56,  13,
	 10,   9,   9, 102, 108, 111,
	 97, 116,  51,  40,  49,  44,
	 48,  46,  53,  44,  48,  41,
	 44,  32,  47,  47,  32,  50,
	 53,  54,  13,  10,   9,   9,
	102, 108, 111,  97, 116,  51,
	 40,  49,  44,  48,  44,  48,
	 41,  44,  32,  47,  47,  32,
	 53,  49,  50,  13,  10,   9,
	  9, 102, 108, 111,  97, 116,
	 51,  40,  49,  44,  48,  44,
	 49,  41,  32,  47,  47,  32,
	 49,  48,  50,  52,  13,  10,
	  9, 125,  59,  13,  10,  13,
	 10,   9, 105, 102,  32,  40,
	108, 101, 118, 101, 108,  32,
	 62,  61,  32,  49,  48,  41,
	 13,  10,   9,   9, 114, 101,
	116, 117, 114, 110,  32, 115,
	116, 111, 112,  80, 111, 105,
	110, 116, 115,  91,  49,  48,
	 93,  59,  13,  10,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 108, 101, 114, 112,
	 40, 115, 116, 111, 112,  80,
	111, 105, 110, 116, 115,  91,
	 40, 105, 110, 116,  41, 108,
	101, 118, 101, 108,  93,  44,
	 32, 115, 116, 111, 112,  80,
	111, 105, 110, 116, 115,  91,
	 40, 105, 110, 116,  41, 108,
	101, 118, 101, 108,  32,  43,
	 32,  49,  93,  44,  32, 108,
	101, 118, 101, 108,  32,  37,
	 32,  49,  41,  59,  13,  10,
	125,  13,  10,  13,  10, 102,
	108, 111,  97, 116,  52,  32,
	109,  97, 105, 110,  40, 102,
	108, 111,  97, 116,  52,  32,
	112, 114, 111, 106,  32,  58,
	 32,  83,  86,  95,  80,  79,
	 83,  73,  84,  73,  79,  78,
	 44,  32, 102, 108, 111,  97,
	116,  50,  32,  67,  32,  58,
	 32,  84,  69,  88,  67,  79,
	 79,  82,  68,  41,  32,  58,
	 32,  83,  86,  95,  84,  65,
	 82,  71,  69,  84,  13,  10,
	123,  13,  10,   9, 105, 110,
	116,  32, 119, 105, 100, 116,
	104,  44,  32, 104, 101, 105,
	103, 104, 116,  59,  13,  10,
	  9, 116, 101, 120,  46,  71,
	101, 116,  68, 105, 109, 101,
	110, 115, 105, 111, 110, 115,
	 40, 119, 105, 100, 116, 104,
	 44,  32, 104, 101, 105, 103,
	104, 116,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 102, 108, 111,  97,
	116,  52,  40,  71, 101, 116,
	 67, 111, 108, 111, 114,  40,
	116, 101, 120,  91, 117, 105,
	110, 116,  50,  40, 119, 105,
	100, 116, 104,  42,  67,  46,
	120,  44,  32, 104, 101, 105,
	103, 104, 116,  42,  67,  46,
	121,  41,  93,  41,  44,  49,
	 41,  59,  13,  10, 125,   0,
	  7,   0,   0,   0,   0,   0,
	  0,   0,  75,   0,   0,   0,
	150,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	 76,   0,   0,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	148,  46,  49,   1, 250, 103,
	166,  92,   1,   0,   0,   0,
	187,  24, 125, 247, 150, 195,
	117,  70, 168, 206,  99, 144,
	125,  61,  40, 142, 119,   0,
	  0,   0,  47,  76, 105, 110,
	107,  73, 110, 102, 111,   0,
	 47, 110,  97, 109, 101, 115,
	  0,  47, 115, 114,  99,  47,
	104, 101,  97, 100, 101, 114,
	 98, 108, 111,  99, 107,   0,
	 47, 115, 114,  99,  47, 102,
	105, 108, 101, 115,  47,  99,
	 58,  92, 117, 115, 101, 114,
	115,  92, 108, 108, 101, 111,
	110,  97, 114, 116,  92, 100,
	101, 115, 107, 116, 111, 112,
	 92, 112, 104, 100,  32, 112,
	114, 111, 106, 101,  99, 116,
	 92,  99,  97,  52, 103,  92,
	115, 104,  97, 100, 101, 114,
	115,  92, 100, 114,  97, 119,
	 99, 111, 109, 112, 108, 101,
	120, 105, 116, 121,  95, 112,
	115,  46, 104, 108, 115, 108,
	  0,   4,   0,   0,   0,   6,
	  0,   0,   0,   1,   0,   0,
	  0,  27,   0,   0,   0,   0,
	  0,   0,   0,  34,   0,   0,
	  0,   8,   0,   0,   0,  17,
	  0,   0,   0,   7,   0,   0,
	  0,  10,   0,   0,   0,   6,
	  0,   0,   0,   0,   0,   0,
	  0,   5,   0,   0,   0,   0,
	  0,   0,   0, 220,  81,  51,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  16,   0,   0,   0,
	 32,   0,   0,   0, 211,   0,
	  0,   0,  16,   1,   0,   0,
	 95,   1,   0,   0,  80,   0,
	  0,   0,   0,   0,   0,   0,
	  2,   4,   0,   0, 128,   0,
	  0,   0,  59,   3,   0,   0,
	240,  12,   0,   0,  60,   0,
	  0,   0,  12,   0,   0,   0,
	 40,   0,   0,   0,  44,   2,
	  0,   0,  44,   0,   0,   0,
	 44,   0,   0,   0,   3,   0,
	  0,   0,  29,   0,   0,   0,
	 19,   0,   0,   0,  26,   0,
	  0,   0,   6,   0,   0,   0,
	 10,   0,   0,   0,  27,   0,
	  0,   0,  28,   0,   0,   0,
	 11,   0,   0,   0,   8,   0,
	  0,   0,   9,   0,   0,   0,
	 12,   0,   0,   0,  13,   0,
	  0,   0,  14,   0,   0,   0,
	 15,   0,   0,   0,  16,   0,
	  0,   0,  17,   0,   0,   0,
	 18,   0,   0,   0,   7,   0,
	  0,   0,  20,   0,   0,   0,
	 21,   0,   0,   0,  22,   0,
	  0,   0,  23,   0,   0,   0,
	 25,   0,   0,   0,  24,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  30,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0
};
#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// tex                               texture  float4          2d             t0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float       
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
dcl_globalFlags refactoringAllowed | skipOptimization
dcl_resource_texture2d(float, float, float, float) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 2
//
// Initial variable locations:
//   v0.x <- proj.x; v0.y <- proj.y; v0.z <- proj.z; v0.w <- proj.w; 
//   v1.x <- C.x; v1.y <- C.y; 
//   o0.x <- <main return value>.x; o0.y <- <main return value>.y; o0.z <- <main return value>.z; o0.w <- <main return value>.w
//
#line 10 "C:\Users\lleonart\Desktop\PhD Project\CA4G\Shaders\DrawScreen_PS.hlsl"
resinfo_indexable(texture2d)(float, float, float, float)_uint r0.xy, l(0), t0.xyzw
mov r0.x, r0.x  // r0.x <- w
mov r0.y, r0.y  // r0.y <- h

#line 12
utof r1.x, r0.x
utof r1.y, r0.y
mul r0.xy, r1.xyxx, v1.xyxx
ftou r0.xy, r0.xyxx
mov r0.zw, l(0, 0, 0, 0)
ld_indexable(texture2d)(float, float, float, float) o0.xyzw, r0.xyzw, t0.xyzw
ret
// Approximately 10 instruction slots used
#endif

const BYTE cso_DrawScreen_PS[] =
{
	 68,  88,  66,  67,  85,  59,
	251, 103,   7,  91, 135,  16,
	163,  42, 196, 152, 113, 122,
	 91, 124,   1,   0,   0,   0,
	 40,  57,   0,   0,   6,   0,
	  0,   0,  56,   0,   0,   0,
	200,   0,   0,   0,  32,   1,
	  0,   0,  84,   1,   0,   0,
	132,   2,   0,   0,  32,   3,
	  0,   0,  82,  68,  69,  70,
	136,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  1,   0,   0,   0,  60,   0,
	  0,   0,   0,   5, 255, 255,
	  5,   1,   0,   0,  96,   0,
	  0,   0,  82,  68,  49,  49,
	 60,   0,   0,   0,  24,   0,
	  0,   0,  32,   0,   0,   0,
	 40,   0,   0,   0,  36,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   0,   0,   0,  92,   0,
	  0,   0,   2,   0,   0,   0,
	  5,   0,   0,   0,   4,   0,
	  0,   0, 255, 255, 255, 255,
	  0,   0,   0,   0,   1,   0,
	  0,   0,  13,   0,   0,   0,
	116, 101, 120,   0,  77, 105,
	 99, 114, 111, 115, 111, 102,
	116,  32,  40,  82,  41,  32,
	 72,  76,  83,  76,  32,  83,
	104,  97, 100, 101, 114,  32,
	 67, 111, 109, 112, 105, 108,
	101, 114,  32,  49,  48,  46,
	 49,   0,  73,  83,  71,  78,
	 80,   0,   0,   0,   2,   0,
	  0,   0,   8,   0,   0,   0,
	 56,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,  15,   0,   0,   0,
	 68,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   1,   0,
	  0,   0,   3,   3,   0,   0,
	 83,  86,  95,  80,  79,  83,
	 73,  84,  73,  79,  78,   0,
	 84,  69,  88,  67,  79,  79,
	 82,  68,   0, 171, 171, 171,
	 79,  83,  71,  78,  44,   0,
	  0,   0,   1,   0,   0,   0,
	  8,   0,   0,   0,  32,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	 15,   0,   0,   0,  83,  86,
	 95,  84,  65,  82,  71,  69,
	 84,   0, 171, 171,  83,  72,
	 69,  88,  40,   1,   0,   0,
	 80,   0,   0,   0,  74,   0,
	  0,   0, 106, 136,   0,   1,
	 88,  24,   0,   4,   0, 112,
	 16,   0,   0,   0,   0,   0,
	 85,  85,   0,   0,  98,  16,
	  0,   3,  50,  16,  16,   0,
	  1,   0,   0,   0, 101,   0,
	  0,   3, 242,  32,  16,   0,
	  0,   0,   0,   0, 104,   0,
	  0,   2,   2,   0,   0,   0,
	 61,  16,   0, 137, 194,   0,
	  0, 128,  67,  85,  21,   0,
	 50,   0,  16,   0,   0,   0,
	  0,   0,   1,  64,   0,   0,
	  0,   0,   0,   0,  70, 126,
	 16,   0,   0,   0,   0,   0,
	 54,   0,   0,   5,  18,   0,
	 16,   0,   0,   0,   0,   0,
	 10,   0,  16,   0,   0,   0,
	  0,   0,  54,   0,   0,   5,
	 34,   0,  16,   0,   0,   0,
	  0,   0,  26,   0,  16,   0,
	  0,   0,   0,   0,  86,   0,
	  0,   5,  18,   0,  16,   0,
	  1,   0,   0,   0,  10,   0,
	 16,   0,   0,   0,   0,   0,
	 86,   0,   0,   5,  34,   0,
	 16,   0,   1,   0,   0,   0,
	 26,   0,  16,   0,   0,   0,
	  0,   0,  56,   0,   0,   7,
	 50,   0,  16,   0,   0,   0,
	  0,   0,  70,   0,  16,   0,
	  1,   0,   0,   0,  70,  16,
	 16,   0,   1,   0,   0,   0,
	 28,   0,   0,   5,  50,   0,
	 16,   0,   0,   0,   0,   0,
	 70,   0,  16,   0,   0,   0,
	  0,   0,  54,   0,   0,   8,
	194,   0,  16,   0,   0,   0,
	  0,   0,   2,  64,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  45,   0,
	  0, 137, 194,   0,   0, 128,
	 67,  85,  21,   0, 242,  32,
	 16,   0,   0,   0,   0,   0,
	 70,  14,  16,   0,   0,   0,
	  0,   0,  70, 126,  16,   0,
	  0,   0,   0,   0,  62,   0,
	  0,   1,  83,  84,  65,  84,
	148,   0,   0,   0,  10,   0,
	  0,   0,   2,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   3,   0,   0,   0,
	  0,   0,   0,   0,   3,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  83,  80,  68,  66,
	  0,  54,   0,   0,  77, 105,
	 99, 114, 111, 115, 111, 102,
	116,  32,  67,  47,  67,  43,
	 43,  32,  77,  83,  70,  32,
	 55,  46,  48,  48,  13,  10,
	 26,  68,  83,   0,   0,   0,
	  0,   2,   0,   0,   2,   0,
	  0,   0,  27,   0,   0,   0,
	136,   0,   0,   0,   0,   0,
	  0,   0,  23,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	192, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255,  56,   0,   0, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255, 255, 255, 255,   5,   0,
	  0,   0,  32,   0,   0,   0,
	 60,   0,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	  0,   0,   0,   0,   6,   0,
	  0,   0,   5,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  3,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 148,  46,  49,   1,
	251, 103, 166,  92,   1,   0,
	  0,   0, 190, 221,   2,  60,
	168,  64, 148,  64, 179, 104,
	 16, 246, 117, 235, 191, 175,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 220,  81,
	 51,   1,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  68,  51,
	 68,  83,  72,  68,  82,   0,
	 40,   1,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  32,   0,
	  0,  96,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	198,  90,   0,   0, 117, 131,
	  1,   0, 178, 211,   1,   0,
	 65,  36,   1,   0, 137, 231,
	  1,   0,   2, 209,   1,   0,
	109,  24,   1,   0,   9, 241,
	  2,   0,   0,  16,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  84, 101, 120, 116,
	117, 114, 101,  50,  68,  60,
	102, 108, 111,  97, 116,  52,
	 62,  32, 116, 101, 120,  32,
	 58,  32, 114, 101, 103, 105,
	115, 116, 101, 114,  32,  40,
	116,  48,  41,  59,  13,  10,
	 13,  10,  47,  47,  99,  98,
	117, 102, 102, 101, 114,  32,
	 84, 114,  97, 110, 115, 102,
	111, 114, 109, 105, 110, 103,
	 32,  58,  32, 114, 101, 103,
	105, 115, 116, 101, 114,  32,
	 40,  98,  48,  41,  32, 123,
	 13,  10,  47,  47,   9, 114,
	111, 119,  95, 109,  97, 106,
	111, 114,  32, 109,  97, 116,
	114, 105, 120,  32,  84, 114,
	 97, 110, 115, 102, 111, 114,
	109,  59,  13,  10,  47,  47,
	125,  13,  10,  13,  10, 102,
	108, 111,  97, 116,  52,  32,
	109,  97, 105, 110,  40, 102,
	108, 111,  97, 116,  52,  32,
	112, 114, 111, 106,  32,  58,
	 32,  83,  86,  95,  80,  79,
	 83,  73,  84,  73,  79,  78,
	 44,  32, 102, 108, 111,  97,
	116,  50,  32,  67,  32,  58,
	 32,  84,  69,  88,  67,  79,
	 79,  82,  68,  41,  32,  58,
	 32,  83,  86,  95,  84,  65,
	 82,  71,  69,  84,  13,  10,
	123,  13,  10,   9, 117, 105,
	110, 116,  32, 119,  44, 104,
	 59,  13,  10,   9, 116, 101,
	120,  46,  71, 101, 116,  68,
	105, 109, 101, 110, 115, 105,
	111, 110, 115,  40, 119,  44,
	 32, 104,  41,  59,  13,  10,
	  9,  47,  47, 114, 101, 116,
	117, 114, 110,  32, 109, 117,
	108,  40, 116, 101, 120,  46,
	 83,  97, 109, 112, 108, 101,
	 40,  83,  97, 109, 112,  44,
	 32,  67,  41,  44,  32,  84,
	114,  97, 110, 115, 102, 111,
	114, 109,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 116, 101, 120,  91,
	117, 105, 110, 116,  50,  40,
	119,  44, 104,  41,  42,  67,
	 93,  59,  13,  10, 125,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 254, 239,
	254, 239,   1,   0,   0,   0,
	202,   1,   0,   0,   0,  67,
	 58,  92,  85, 115, 101, 114,
	115,  92, 108, 108, 101, 111,
	110,  97, 114, 116,  92,  68,
	101, 115, 107, 116, 111, 112,
	 92,  80, 104,  68,  32,  80,
	114, 111, 106, 101,  99, 116,
	 92,  67,  65,  52,  71,  92,
	 83, 104,  97, 100, 101, 114,
	115,  92,  68, 114,  97, 119,
	 83,  99, 114, 101, 101, 110,
	 95,  80,  83,  46, 104, 108,
	115, 108,   0,   0,  99,  58,
	 92, 117, 115, 101, 114, 115,
	 92, 108, 108, 101, 111, 110,
	 97, 114, 116,  92, 100, 101,
	115, 107, 116, 111, 112,  92,
	112, 104, 100,  32, 112, 114,
	111, 106, 101,  99, 116,  92,
	 99,  97,  52, 103,  92, 115,
	104,  97, 100, 101, 114, 115,
	 92, 100, 114,  97, 119, 115,
	 99, 114, 101, 101, 110,  95,
	112, 115,  46, 104, 108, 115,
	108,   0,  84, 101, 120, 116,
	117, 114, 101,  50,  68,  60,
	102, 108, 111,  97, 116,  52,
	 62,  32, 116, 101, 120,  32,
	 58,  32, 114, 101, 103, 105,
	115, 116, 101, 114,  32,  40,
	116,  48,  41,  59,  13,  10,
	 13,  10,  47,  47,  99,  98,
	117, 102, 102, 101, 114,  32,
	 84, 114,  97, 110, 115, 102,
	111, 114, 109, 105, 110, 103,
	 32,  58,  32, 114, 101, 103,
	105, 115, 116, 101, 114,  32,
	 40,  98,  48,  41,  32, 123,
	 13,  10,  47,  47,   9, 114,
	111, 119,  95, 109,  97, 106,
	111, 114,  32, 109,  97, 116,
	114, 105, 120,  32,  84, 114,
	 97, 110, 115, 102, 111, 114,
	109,  59,  13,  10,  47,  47,
	125,  13,  10,  13,  10, 102,
	108, 111,  97, 116,  52,  32,
	109,  97, 105, 110,  40, 102,
	108, 111,  97, 116,  52,  32,
	112, 114, 111, 106,  32,  58,
	 32,  83,  86,  95,  80,  79,
	 83,  73,  84,  73,  79,  78,
	 44,  32, 102, 108, 111,  97,
	116,  50,  32,  67,  32,  58,
	 32,  84,  69,  88,  67,  79,
	 79,  82,  68,  41,  32,  58,
	 32,  83,  86,  95,  84,  65,
	 82,  71,  69,  84,  13,  10,
	123,  13,  10,   9, 117, 105,
	110, 116,  32, 119,  44, 104,
	 59,  13,  10,   9, 116, 101,
	120,  46,  71, 101, 116,  68,
	105, 109, 101, 110, 115, 105,
	111, 110, 115,  40, 119,  44,
	 32, 104,  41,  59,  13,  10,
	  9,  47,  47, 114, 101, 116,
	117, 114, 110,  32, 109, 117,
	108,  40, 116, 101, 120,  46,
	 83,  97, 109, 112, 108, 101,
	 40,  83,  97, 109, 112,  44,
	 32,  67,  41,  44,  32,  84,
	114,  97, 110, 115, 102, 111,
	114, 109,  41,  59,  13,  10,
	  9, 114, 101, 116, 117, 114,
	110,  32, 116, 101, 120,  91,
	117, 105, 110, 116,  50,  40,
	119,  44, 104,  41,  42,  67,
	 93,  59,  13,  10, 125,   0,
	  7,   0,   0,   0,   0,   0,
	  0,   0,  71,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,  72,   0,   0,   0,
	142,   0,   0,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 27, 226,  48,   1, 128,   0,
	  0,   0,  18,  54, 150, 102,
	 36, 235, 212,   1,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   2,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 72,   0,   0,   0,  40,   0,
	  0,   0,  27, 226,  48,   1,
	103, 155, 154, 125,  59,   1,
	  0,   0,   1,   0,   0,   0,
	 71,   0,   0,   0,  72,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,
	 66,   0,  60,  17,  16,   1,
	  0,   0,   0,   1,  10,   0,
	  1,   0, 132,   0,  99,  69,
	 10,   0,   1,   0, 132,   0,
	 99,  69,  77, 105,  99, 114,
	111, 115, 111, 102, 116,  32,
	 40,  82,  41,  32,  72,  76,
	 83,  76,  32,  83, 104,  97,
	100, 101, 114,  32,  67, 111,
	109, 112, 105, 108, 101, 114,
	 32,  49,  48,  46,  49,   0,
	  0,   0,  54,   0,  61,  17,
	  1, 104, 108, 115, 108,  70,
	108,  97, 103, 115,   0,  48,
	120,  53,   0, 104, 108, 115,
	108,  84,  97, 114, 103, 101,
	116,   0, 112, 115,  95,  53,
	 95,  48,   0, 104, 108, 115,
	108,  69, 110, 116, 114, 121,
	  0, 109,  97, 105, 110,   0,
	  0,   0,   0,   0,  42,   0,
	 16,  17,   0,   0,   0,   0,
	184,   2,   0,   0,   0,   0,
	  0,   0, 236,   0,   0,   0,
	  0,   0,   0,   0, 236,   0,
	  0,   0,   4,  16,   0,   0,
	 60,   0,   0,   0,   1,   0,
	160, 109,  97, 105, 110,   0,
	 42,   0,  62,  17,   0,  16,
	  0,   0,   9,   0, 112, 114,
	111, 106,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   0,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 236,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   4,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 236,   0,   4,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,   8,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 236,   0,   8,   0,
	  0,   0,  22,   0,  80,  17,
	  1,   0,   5,   0,  12,   0,
	  4,   0,  60,   0,   0,   0,
	  1,   0, 236,   0,  12,   0,
	  0,   0,  42,   0,  62,  17,
	  1,  16,   0,   0,   9,   0,
	 67,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   1,   0,   5,   0,
	  0,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	 16,   0,   0,   0,  22,   0,
	 80,  17,   1,   0,   5,   0,
	  4,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	 20,   0,   0,   0,  58,   0,
	 62,  17,   3,  16,   0,   0,
	136,   0,  60, 109,  97, 105,
	110,  32, 114, 101, 116, 117,
	114, 110,  32, 118,  97, 108,
	117, 101,  62,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  0,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	  0,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  4,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	  4,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	  8,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	  8,   0,   0,   0,  22,   0,
	 80,  17,   2,   0,   5,   0,
	 12,   0,   4,   0,  60,   0,
	  0,   0,   1,   0, 236,   0,
	 12,   0,   0,   0,  42,   0,
	 62,  17, 117,   0,   0,   0,
	  0,   0, 119,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 22,   0,  80,  17,   0,   0,
	  1,   0,   0,   0,   4,   0,
	116,   0,   0,   0,   1,   0,
	180,   0,   0,   0,   0,   0,
	 42,   0,  62,  17, 117,   0,
	  0,   0,   0,   0, 104,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,  80,  17,
	  0,   0,   1,   0,   0,   0,
	  4,   0, 136,   0,   0,   0,
	  1,   0, 160,   0,   4,   0,
	  0,   0,   2,   0,   6,   0,
	244,   0,   0,   0,  24,   0,
	  0,   0,   1,   0,   0,   0,
	 16,   1, 185,  17,  16, 126,
	216,  30, 223,  43,  12,  49,
	229, 143, 134, 240, 181,  38,
	  0,   0, 242,   0,   0,   0,
	  8,   1,   0,   0,   0,   0,
	  0,   0,   1,   0,   1,   0,
	 40,   1,   0,   0,   0,   0,
	  0,   0,  20,   0,   0,   0,
	252,   0,   0,   0,  60,   0,
	  0,   0,  10,   0,   0, 128,
	 60,   0,   0,   0,  10,   0,
	  0,   0,  96,   0,   0,   0,
	 10,   0,   0, 128,  96,   0,
	  0,   0,  10,   0,   0,   0,
	116,   0,   0,   0,  10,   0,
	  0, 128, 116,   0,   0,   0,
	 10,   0,   0,   0, 136,   0,
	  0,   0,  12,   0,   0, 128,
	136,   0,   0,   0,  12,   0,
	  0,   0, 156,   0,   0,   0,
	 12,   0,   0, 128, 156,   0,
	  0,   0,  12,   0,   0,   0,
	176,   0,   0,   0,  12,   0,
	  0, 128, 176,   0,   0,   0,
	 12,   0,   0,   0, 204,   0,
	  0,   0,  12,   0,   0, 128,
	204,   0,   0,   0,  12,   0,
	  0,   0, 224,   0,   0,   0,
	 12,   0,   0, 128, 224,   0,
	  0,   0,  12,   0,   0,   0,
	  0,   1,   0,   0,  12,   0,
	  0, 128,   0,   1,   0,   0,
	 12,   0,   0,   0,  36,   1,
	  0,   0,  12,   0,   0, 128,
	 36,   1,   0,   0,  12,   0,
	  0,   0,   2,   0,  25,   0,
	  2,   0,  24,   0,   2,   0,
	 25,   0,   2,   0,  24,   0,
	  2,   0,  25,   0,   2,   0,
	 24,   0,   2,   0,  26,   0,
	 13,   0,  24,   0,   2,   0,
	 26,   0,  13,   0,  24,   0,
	  2,   0,  26,   0,  13,   0,
	 24,   0,   2,   0,  26,   0,
	  9,   0,  25,   0,   2,   0,
	 26,   0,   9,   0,  25,   0,
	  2,   0,  26,   0,   9,   0,
	 25,   0,   2,   0,  26,   0,
	  2,   0,  26,   0, 246,   0,
	  0,   0,   4,   0,   0,   0,
	  0,   0,   0,   0,   8,   0,
	  0,   0,   0,   0,   0,   0,
	 20,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  11, 202,  49,   1,
	 56,   0,   0,   0,   0,  16,
	  0,   0,   8,  16,   0,   0,
	132,   0,   0,   0,  10,   0,
	255, 255,   4,   0,   0,   0,
	255, 255,   3,   0,   0,   0,
	  0,   0,  32,   0,   0,   0,
	 32,   0,   0,   0,   8,   0,
	  0,   0,  40,   0,   0,   0,
	  0,   0,   0,   0,  22,   0,
	 27,  21,  64,   0,   0,   0,
	  4,   0,   0,   0,  16,   0,
	102, 108, 111,  97, 116,  52,
	  0, 243, 242, 241,  22,   0,
	 27,  21,  64,   0,   0,   0,
	  2,   0,   0,   0,   8,   0,
	102, 108, 111,  97, 116,  50,
	  0, 243, 242, 241,  14,   0,
	  1,  18,   2,   0,   0,   0,
	  0,  16,   0,   0,   1,  16,
	  0,   0,  10,   0,  24,  21,
	  0,  16,   0,   0,   1,   0,
	  1,   0,  14,   0,   8,  16,
	  3,  16,   0,   0,  23,   0,
	  2,   0,   2,  16,   0,   0,
	 14,   0,  23,  21,   0,  16,
	  0,   0,   3,   2, 144,   2,
	  0,   0, 242, 241,  10,   0,
	 24,  21,   5,  16,   0,   0,
	  1,   0,   1,   0,  10,   0,
	 24,  21,   6,  16,   0,   0,
	  1,   0,   0,   2,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  11, 202,
	 49,   1,  56,   0,   0,   0,
	  0,  16,   0,   0,   0,  16,
	  0,   0,   0,   0,   0,   0,
	 11,   0, 255, 255,   4,   0,
	  0,   0, 255, 255,   3,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	255, 255, 255, 255,  26,   9,
	 47, 241,  16,   0,   0,   0,
	 12,   2,   0,   0,  21,   0,
	  0,   0,   1,   0,   0,   0,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  16,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  32,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 12,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,  18,   0,
	 37,  17,   0,   0,   0,   0,
	128,   0,   0,   0,   1,   0,
	109,  97, 105, 110,   0,   0,
	 22,   0,  81,  17,   7,  16,
	  0,   0,   7,   0, 255, 255,
	255, 255,   0,   0, 255, 255,
	255, 255, 116, 101, 120,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 16,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 255, 255,
	255, 255,  26,   9,  47, 241,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	119,   9,  49,   1,   1,   0,
	  0,   0,  13,   0,   0, 142,
	 14,   0,  63,  92,  15,   0,
	  0,   0,  76,   0,   0,   0,
	 32,   0,   0,   0,  44,   0,
	  0,   0,  84,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,   0,   0,
	 25,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,
	 40,   1,   0,   0,  32,   0,
	  0,  96,   0,   0, 191, 175,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   2,   0,   9,   0,
	188,   2,   0,   0,   0,   0,
	  0,   0,  60,   1,   0,   0,
	  1,   0,   0,   0, 120, 200,
	204,   2,   0,   0,   0,   0,
	  0,   0,   0,   0, 109,  97,
	105, 110,   0, 110, 111, 110,
	101,   0,   0,   0,  45, 186,
	 46, 241,   1,   0,   0,   0,
	  0,   0,   0,   0,  40,   1,
	  0,   0,  32,   0,   0,  96,
	  0,   0, 191, 175,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  2,   0,   2,   0,   7,   0,
	  0,   0,   0,   0,   1,   0,
	255, 255, 255, 255,   0,   0,
	  0,   0,  40,   1,   0,   0,
	  8,   2,   0,   0,   0,   0,
	  0,   0, 255, 255, 255, 255,
	  0,   0,   0,   0, 255, 255,
	255, 255,   1,   0,   1,   0,
	  0,   0,   1,   0,   0,   0,
	  0,   0,  67,  58,  92,  85,
	115, 101, 114, 115,  92, 108,
	108, 101, 111, 110,  97, 114,
	116,  92,  68, 101, 115, 107,
	116, 111, 112,  92,  80, 104,
	 68,  32,  80, 114, 111, 106,
	101,  99, 116,  92,  67,  65,
	 52,  71,  92,  83, 104,  97,
	100, 101, 114, 115,  92,  68,
	114,  97, 119,  83,  99, 114,
	101, 101, 110,  95,  80,  83,
	 46, 104, 108, 115, 108,   0,
	  0,   0, 254, 239, 254, 239,
	  1,   0,   0,   0,   1,   0,
	  0,   0,   0,   1,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255,  12,   0, 255, 255, 255,
	255, 255, 255, 255, 255, 255,
	255,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 148,  46,
	 49,   1, 251, 103, 166,  92,
	  1,   0,   0,   0, 190, 221,
	  2,  60, 168,  64, 148,  64,
	179, 104,  16, 246, 117, 235,
	191, 175, 115,   0,   0,   0,
	 47,  76, 105, 110, 107,  73,
	110, 102, 111,   0,  47, 110,
	 97, 109, 101, 115,   0,  47,
	115, 114,  99,  47, 104, 101,
	 97, 100, 101, 114,  98, 108,
	111,  99, 107,   0,  47, 115,
	114,  99,  47, 102, 105, 108,
	101, 115,  47,  99,  58,  92,
	117, 115, 101, 114, 115,  92,
	108, 108, 101, 111, 110,  97,
	114, 116,  92, 100, 101, 115,
	107, 116, 111, 112,  92, 112,
	104, 100,  32, 112, 114, 111,
	106, 101,  99, 116,  92,  99,
	 97,  52, 103,  92, 115, 104,
	 97, 100, 101, 114, 115,  92,
	100, 114,  97, 119, 115,  99,
	114, 101, 101, 110,  95, 112,
	115,  46, 104, 108, 115, 108,
	  0,   4,   0,   0,   0,   6,
	  0,   0,   0,   1,   0,   0,
	  0,  58,   0,   0,   0,   0,
	  0,   0,   0,  17,   0,   0,
	  0,   7,   0,   0,   0,  10,
	  0,   0,   0,   6,   0,   0,
	  0,   0,   0,   0,   0,   5,
	  0,   0,   0,  34,   0,   0,
	  0,   8,   0,   0,   0,   0,
	  0,   0,   0, 220,  81,  51,
	  1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	 16,   0,   0,   0,  32,   0,
	  0,   0, 207,   0,   0,   0,
	188,   0,   0,   0,  91,   1,
	  0,   0,  56,   0,   0,   0,
	  0,   0,   0,   0, 250,   1,
	  0,   0, 128,   0,   0,   0,
	 59,   1,   0,   0,   4,   4,
	  0,   0,  40,   0,   0,   0,
	  0,   0,   0,   0,  40,   0,
	  0,   0,  44,   2,   0,   0,
	 44,   0,   0,   0,  44,   0,
	  0,   0,   3,   0,   0,   0,
	 21,   0,   0,   0,  14,   0,
	  0,   0,  20,   0,   0,   0,
	 15,   0,   0,   0,   9,   0,
	  0,   0,  10,   0,   0,   0,
	  8,   0,   0,   0,  11,   0,
	  0,   0,  12,   0,   0,   0,
	 13,   0,   0,   0,   7,   0,
	  0,   0,   6,   0,   0,   0,
	 16,   0,   0,   0,  17,   0,
	  0,   0,  19,   0,   0,   0,
	 18,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,  22,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0
};



#pragma endregion

namespace CA4G {

	// Represents the shader stage a resource is bound to.
	enum ShaderType {
		// Resource is bound to pixel and vertex stages in the same slot.
		ShaderType_Any = D3D12_SHADER_VISIBILITY_ALL,
		// Resource is bound to vertex stage.
		ShaderType_Vertex = D3D12_SHADER_VISIBILITY_VERTEX,
		// Resource is bound to geometry stage.
		ShaderType_Geometry = D3D12_SHADER_VISIBILITY_GEOMETRY,
		// Resource is bound to pixel stage.
		ShaderType_Pixel = D3D12_SHADER_VISIBILITY_PIXEL,
		// Resource is bound to hull stage.
		ShaderType_Hull = D3D12_SHADER_VISIBILITY_HULL,
		// Resource is bound to domain stage.
		ShaderType_Domain = D3D12_SHADER_VISIBILITY_DOMAIN
	};

	// Represents the element type of a vertex field.
	enum VertexElementType {
		// Each component of this field is a signed integer
		VertexElementType_Int,
		// Each component of this field is an unsigned integer
		VertexElementType_UInt,
		// Each component of this field is a floating value
		VertexElementType_Float
	};


#pragma region Vertex Description

	static DXGI_FORMAT TYPE_VS_COMPONENTS_FORMATS[3][4]{
		/*Int*/{ DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32B32_SINT,DXGI_FORMAT_R32G32B32A32_SINT },
		/*Unt*/{ DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32B32_UINT,DXGI_FORMAT_R32G32B32A32_UINT },
		/*Float*/{ DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT },
	};

	// Basic struct for constructing vertex element descriptions
	struct VertexElement {
		// Type for each field component
		const VertexElementType Type;
		// Number of components
		const int Components;
		// String with the semantic
		LPCSTR const Semantic;
		// Index for indexed semantics
		const int SemanticIndex;
		// Buffer slot this field will be contained.
		const int Slot;
	public:
		constexpr VertexElement(
			VertexElementType Type,
			int Components,
			LPCSTR const Semantic,
			int SemanticIndex = 0,
			int Slot = 0
		) :Type(Type), Components(Components), Semantic(Semantic), SemanticIndex(SemanticIndex), Slot(Slot)
		{
		}
		// Creates a Dx12 description using this information.
		D3D12_INPUT_ELEMENT_DESC createDesc(int offset, int &size) const {
			D3D12_INPUT_ELEMENT_DESC d = {};
			d.AlignedByteOffset = offset;
			d.Format = TYPE_VS_COMPONENTS_FORMATS[(int)Type][Components - 1];
			d.InputSlot = Slot;
			d.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			d.SemanticIndex = SemanticIndex;
			d.SemanticName = Semantic;
			size += 4 * Components;
			return d;
		}
	};

#pragma endregion

#pragma region Pipeline bindings

	// Used for default initialization similar to d3dx12 usage
	struct DefaultStateValue {

		operator D3D12_RASTERIZER_DESC () {
			D3D12_RASTERIZER_DESC d = {};
			d.FillMode = D3D12_FILL_MODE_SOLID;
			d.CullMode = D3D12_CULL_MODE_NONE;
			d.FrontCounterClockwise = FALSE;
			d.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			d.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			d.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			d.DepthClipEnable = TRUE;
			d.MultisampleEnable = FALSE;
			d.AntialiasedLineEnable = FALSE;
			d.ForcedSampleCount = 0;
			d.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			return d;
		}

		operator D3D12_DEPTH_STENCIL_DESC () {
			D3D12_DEPTH_STENCIL_DESC d = { };

			d.DepthEnable = FALSE;
			d.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			d.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			d.StencilEnable = FALSE;
			d.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			d.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			d.FrontFace = defaultStencilOp;
			d.BackFace = defaultStencilOp;

			return d;
		}

		operator D3D12_BLEND_DESC () {
			D3D12_BLEND_DESC d = { };
			d.AlphaToCoverageEnable = FALSE;
			d.IndependentBlendEnable = FALSE;
			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
				FALSE,FALSE,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};
			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				d.RenderTarget[i] = defaultRenderTargetBlendDesc;
			return d;
		}

		operator DXGI_SAMPLE_DESC() {
			DXGI_SAMPLE_DESC d = { };
			d.Count = 1;
			d.Quality = 0;
			return d;
		}

		operator D3D12_DEPTH_STENCIL_DESC1 () {
			D3D12_DEPTH_STENCIL_DESC1 _Description = { };
			_Description.DepthEnable = TRUE;
			_Description.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			_Description.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			_Description.StencilEnable = FALSE;
			_Description.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			_Description.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			_Description.FrontFace = defaultStencilOp;
			_Description.BackFace = defaultStencilOp;
			_Description.DepthBoundsTestEnable = false;
			return _Description;
		}
	};

	struct DefaultSampleMask {
		operator UINT()
		{
			return UINT_MAX;
		}
	};

	struct DefaultTopology {
		operator D3D12_PRIMITIVE_TOPOLOGY_TYPE() {
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
	};

#pragma region Pipeline Subobject states

	// Adapted from d3dx12 class
	template<typename Description, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultValue = Description>
	struct alignas(void*) PipelineSubobjectState {
		template<typename ...A> friend class PipelineBindings;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _Type;
		Description _Description;

		inline Description& getDescription() {
			return _Description;
		}

		PipelineSubobjectState() noexcept : _Type(Type), _Description(DefaultValue()) {}
		PipelineSubobjectState(Description const& d) : _Type(Type), _Description(d) {}
		PipelineSubobjectState& operator=(Description const& i) { _Description = i; return *this; }
		operator Description() const { return _Description; }
		operator Description&() { return _Description; }

		static const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE PipelineState_Type = Type;
	};

	struct DebugStateManager : public PipelineSubobjectState< D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS> {
		void Debug() { _Description = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG; }
		void NoDebug() { _Description = D3D12_PIPELINE_STATE_FLAG_NONE; }
	};

	struct NodeMaskStateManager : public PipelineSubobjectState< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK> {
		void ExecutionAt(int node) {
			_Description = 1 << node;
		}
		void ExecutionSingleAdapter() {
			_Description = 0;
		}
	};

	struct RootSignatureStateManager : public PipelineSubobjectState< ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> {
		template<typename ...A> friend class PipelineBindings;
	private:
		void SetRootSignature(ID3D12RootSignature* rootSignature) {
			_Description = rootSignature;
		}
	};

	struct InputLayoutStateManager : public PipelineSubobjectState< D3D12_INPUT_LAYOUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT> {
		void InputLayout(std::initializer_list<VertexElement> elements) {
			if (_Description.pInputElementDescs != nullptr)
				delete[] _Description.pInputElementDescs;
			auto layout = new D3D12_INPUT_ELEMENT_DESC[elements.size()];
			int offset = 0;
			auto current = elements.begin();
			for (int i = 0; i < elements.size(); i++)
			{
				int size = 0;
				layout[i] = current->createDesc(offset, size);
				offset += size;
				current++;
			}
			_Description.pInputElementDescs = layout;
			_Description.NumElements = elements.size();
		}

		template<int N>
		void InputLayout(VertexElement(&elements)[N]) {
			if (_Description.pInputElementDescs != nullptr)
				delete[] _Description.pInputElementDescs;
			auto layout = new D3D12_INPUT_ELEMENT_DESC[N];
			int offset = 0;
			for (int i = 0; i < N; i++)
			{
				int size = 0;
				layout[i] = elements[i]->createDesc(offset, size);
				offset += size;
			}
			_Description.pInputElementDescs = layout;
			_Description.NumElements = N;
		}
	};

	struct IndexBufferStripStateManager : public PipelineSubobjectState< D3D12_INDEX_BUFFER_STRIP_CUT_VALUE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE> {
		void DisableIndexBufferCut() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		}
		void IndexBufferCutAt32BitMAX() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		}
		void IndexBufferCutAt64BitMAX() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		}
	};

	struct PrimitiveTopologyStateManager : public PipelineSubobjectState< D3D12_PRIMITIVE_TOPOLOGY_TYPE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, DefaultTopology> {
		void CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) {
			_Description = type;
		}
		void TrianglesTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		}
		void PointsTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		}
		void LinesTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		}
	};

	template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
	struct ShaderStageStateManager : public PipelineSubobjectState< D3D12_SHADER_BYTECODE, Type> {
	protected:
		void FromBytecode(const D3D12_SHADER_BYTECODE &shaderBytecode) {
			this->_Description = shaderBytecode;
		}
	};

	struct VertexShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS> {
		void VertexShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct PixelShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> {
		void PixelShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct GeometryShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS> {
		void GeometryShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct HullShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS> {
		void HullShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct DomainShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS> {
		void DomainShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct ComputeShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS> {
		void ComputeShader(const D3D12_SHADER_BYTECODE &bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct StreamOutputStateManager : public PipelineSubobjectState< D3D12_STREAM_OUTPUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT> {
	};

	struct BlendingStateManager : public PipelineSubobjectState< D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, DefaultStateValue> {

		void AlphaToCoverage(bool enable = true) {
			_Description.AlphaToCoverageEnable = enable;
		}
		void IndependentBlend(bool enable = true) {
			_Description.IndependentBlendEnable = enable;
		}
		void BlendAtRenderTarget(
			int renderTarget = 0,
			bool enable = true,
			D3D12_BLEND_OP operation = D3D12_BLEND_OP_ADD,
			D3D12_BLEND src = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dst = D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP alphaOperation = D3D12_BLEND_OP_MAX,
			D3D12_BLEND srcAlpha = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dstAlpha = D3D12_BLEND_DEST_ALPHA,
			bool enableLogicOperation = false,
			D3D12_LOGIC_OP logicOperation = D3D12_LOGIC_OP_COPY
		) {
			D3D12_RENDER_TARGET_BLEND_DESC d;
			d.BlendEnable = enable;
			d.BlendOp = operation;
			d.BlendOpAlpha = alphaOperation;
			d.SrcBlend = src;
			d.DestBlend = dst;
			d.SrcBlendAlpha = srcAlpha;
			d.DestBlendAlpha = dstAlpha;
			d.LogicOpEnable = enableLogicOperation;
			d.LogicOp = logicOperation;
			_Description.RenderTarget[renderTarget] = d;
		}
		void BlendForAllRenderTargets(
			bool enable = true,
			D3D12_BLEND_OP operation = D3D12_BLEND_OP_ADD,
			D3D12_BLEND src = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dst = D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP alphaOperation = D3D12_BLEND_OP_MAX,
			D3D12_BLEND srcAlpha = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dstAlpha = D3D12_BLEND_DEST_ALPHA,
			bool enableLogicOperation = false,
			D3D12_LOGIC_OP logicOperation = D3D12_LOGIC_OP_COPY
		) {
			D3D12_RENDER_TARGET_BLEND_DESC d;
			d.BlendEnable = enable;
			d.BlendOp = operation;
			d.BlendOpAlpha = alphaOperation;
			d.SrcBlend = src;
			d.DestBlend = dst;
			d.SrcBlendAlpha = srcAlpha;
			d.DestBlendAlpha = dstAlpha;
			d.LogicOpEnable = enableLogicOperation;
			d.LogicOp = logicOperation;
			for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
				_Description.RenderTarget[i] = d;
		}
		void BlendDisabledAtRenderTarget(int renderTarget) {
			BlendAtRenderTarget(renderTarget, false);
		}
		void BlendDisabled() {
			BlendForAllRenderTargets(false);
		}
	};

	struct DepthStencilStateManager : public PipelineSubobjectState< D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, DefaultStateValue> {

		void NoDepthTest() {
			_Description = {};
		}
		void DepthTest(bool enable = true, bool writeDepth = true, D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_LESS_EQUAL) {
			_Description.DepthEnable = enable;
			_Description.DepthWriteMask = writeDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			_Description.DepthFunc = comparison;
		}
		void StencilTest(bool enable = true, byte readMask = 0xFF, byte writeMask = 0xFF) {
			_Description.StencilEnable = enable;
			_Description.StencilReadMask = readMask;
			_Description.StencilWriteMask = writeMask;
		}
		void StencilOperationAtFront(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.FrontFace.StencilFailOp = fail;
			_Description.FrontFace.StencilDepthFailOp = depthFail;
			_Description.FrontFace.StencilPassOp = pass;
			_Description.FrontFace.StencilFunc = function;
		}
		void StencilOperationAtBack(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.BackFace.StencilFailOp = fail;
			_Description.BackFace.StencilDepthFailOp = depthFail;
			_Description.BackFace.StencilPassOp = pass;
			_Description.BackFace.StencilFunc = function;
		}
	};

	struct DepthStencilWithDepthBoundsStateManager : public PipelineSubobjectState< D3D12_DEPTH_STENCIL_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, DefaultStateValue> {

		void DepthTest(bool enable = true, bool writeDepth = true, D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_LESS_EQUAL) {
			_Description.DepthEnable = enable;
			_Description.DepthWriteMask = writeDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			_Description.DepthFunc = comparison;
		}
		void StencilTest(bool enable = true, byte readMask = 0xFF, byte writeMask = 0xFF) {
			_Description.StencilEnable = enable;
			_Description.StencilReadMask = readMask;
			_Description.StencilWriteMask = writeMask;
		}
		void StencilOperationAtFront(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.FrontFace.StencilFailOp = fail;
			_Description.FrontFace.StencilDepthFailOp = depthFail;
			_Description.FrontFace.StencilPassOp = pass;
			_Description.FrontFace.StencilFunc = function;
		}
		void StencilOperationAtBack(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.BackFace.StencilFailOp = fail;
			_Description.BackFace.StencilDepthFailOp = depthFail;
			_Description.BackFace.StencilPassOp = pass;
			_Description.BackFace.StencilFunc = function;
		}
		void DepthBoundsTest(bool enable) {
			_Description.DepthBoundsTestEnable = enable;
		}
	};

	struct DepthStencilFormatStateManager : public PipelineSubobjectState< DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> {
		void DepthStencilFormat(DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT) {
			_Description = format;
		}
	};

	struct RasterizerStateManager : public PipelineSubobjectState< D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, DefaultStateValue> {

		void AntialiasedLine(bool enable = true, bool multisample = true) {
			_Description.AntialiasedLineEnable = false;
			_Description.MultisampleEnable = multisample;
		}

		void ConservativeRasterization(bool enable = true) {
			_Description.ConservativeRaster = enable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}

		void CullMode(D3D12_CULL_MODE mode = D3D12_CULL_MODE_NONE) {
			_Description.CullMode = mode;
		}

		void FillMode(D3D12_FILL_MODE mode = D3D12_FILL_MODE_SOLID) {
			_Description.FillMode = mode;
		}

		void DepthBias(int depthBias = 1, float slopeScale = 0, float clamp = D3D12_FLOAT32_MAX)
		{
			_Description.DepthBias = depthBias;
			_Description.SlopeScaledDepthBias = slopeScale;
			_Description.DepthBiasClamp = clamp;
		}

		void ForcedSampleCount(UINT value) {
			_Description.ForcedSampleCount = value;
		}
	};

	struct RenderTargetFormatsStateManager : public PipelineSubobjectState< D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> {
		void RenderTargetFormatAt(int slot, DXGI_FORMAT format) {
			if (slot >= _Description.NumRenderTargets)
				_Description.NumRenderTargets = slot + 1;

			_Description.RTFormats[slot] = format;
		}
		void AllRenderTargets(int numberOfRTs, DXGI_FORMAT format) {
			_Description.NumRenderTargets = numberOfRTs;
			for (int i = 0; i < numberOfRTs; i++)
				_Description.RTFormats[i] = format;
		}
	};

	struct MultisamplingStateManager : public PipelineSubobjectState< DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DefaultStateValue> {


		void Multisampling(int count = 1, int quality = 0) {
			_Description.Count = count;
			_Description.Quality = quality;
		}
	};

	struct BlendSampleMaskStateManager : public PipelineSubobjectState< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, DefaultSampleMask> {

		void BlendSampleMask(UINT value = UINT_MAX) {
			_Description = value;
		}
	};

	struct MultiViewInstancingStateManager : public PipelineSubobjectState< D3D12_VIEW_INSTANCING_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, DefaultStateValue> {
		void MultipleViews(std::initializer_list<D3D12_VIEW_INSTANCE_LOCATION> views, D3D12_VIEW_INSTANCING_FLAGS flags = D3D12_VIEW_INSTANCING_FLAG_NONE) {
			_Description.ViewInstanceCount = views.size();
			auto newViews = new D3D12_VIEW_INSTANCE_LOCATION[views.size()];
			auto current = views.begin();
			for (int i = 0; i < views.size(); i++) {
				newViews[i] = *current;
				current++;
			}
			if (_Description.pViewInstanceLocations != nullptr)
				delete[] _Description.pViewInstanceLocations;
			_Description.pViewInstanceLocations = newViews;
		}
	};

	//typedef PipelineSubobjectState< D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>                        CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO;

#pragma endregion

	class IPipelineBindings {
		friend ComputeManager;
		friend GraphicsManager;
		template<typename ...A> friend class PipelineBindings;

		// After initialization this variable is set with the root signature
		ID3D12RootSignature *rootSignature;
		// When pipeline setting is closed, the pipeline state object is created and stored.
		ID3D12PipelineState *pso = nullptr;

		virtual void __OnSet(ComputeManager* manager) = 0;

		virtual void __OnDraw(ComputeManager* manager) = 0;

		virtual void __OnInitialization(gObj<DeviceManager> manager) = 0;
	};

	template<typename PSS, typename R, typename ...A>
	unsigned long  StreamTypeBits() {
		return (1 << (int)PSS::PipelineState_Type) | StreamTypeBits<R, A...>();
	}

	template<typename PSS>
	unsigned long  StreamTypeBits() {
		return (1 << (int)PSS::PipelineState_Type);// | StreamTypeBits<A...>();
	}

	// -- Abstract pipeline object (can be Graphics or Compute)
	// Allows creation of root signatures and leaves abstract the pipeline state object creation
	template<typename ...PSS>
	class PipelineBindings : public IPipelineBindings {

		friend Loading;
		friend GraphicsManager;

		// Represents a binding of a resource (or resource array) to a shader slot.
		struct SlotBinding {
			// Determines wich shader stage this resource is bound to
			D3D12_SHADER_VISIBILITY visibility;
			// Determines the type of the resource
			D3D12_DESCRIPTOR_RANGE_TYPE type;
			// Determines the slot for binding
			int slot;
			// Determines the dimension of the bound resource
			D3D12_RESOURCE_DIMENSION Dimension;
			// Gets the pointer to a resource field (pointer to ResourceView)
			void* ptrToResource;
			// Gets a pointer to a number if a resource array is bound
			int* ptrToCount;
			// Determines the space for binding
			int space;
		};

		// Used to collect all global constant, shader or unordered views bindings
		list<SlotBinding> __GlobalsCSU;
		// Used to collect all local constant, shader or unordered views bindings
		list<SlotBinding> __LocalsCSU;
		// Used to collect all global sampler bindings
		list<SlotBinding> __GlobalsSamplers;
		// Used to collect all local sampler bindings
		list<SlotBinding> __LocalsSamplers;

		// Used to collect all render targets bindings
		gObj<Texture2D> *RenderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		// Used to collect all render targets bindings
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		// Gets or sets the maxim slot of bound render target
		int RenderTargetMax = 0;
		// Gets the bound depth buffer resource
		gObj<Texture2D>* DepthBufferField = nullptr;

		// Used to collect static samplers
		D3D12_STATIC_SAMPLER_DESC Static_Samplers[32];
		// Gets or sets the maxim sampler slot used
		int StaticSamplerMax = 0;

		// Use to change between global and local list during collecting
		list<SlotBinding> *__CurrentLoadingCSU;
		// Use to change between global and local list during collecting
		list<SlotBinding> *__CurrentLoadingSamplers;

		gObj<DeviceManager> manager;

		unsigned long StreamBits = 0;

		bool HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type) {
			return (StreamBits & (1 << (int)type)) != 0;
		}

#pragma region Query to know if some state is active
		// set to true if a vertex shader is set
		bool Using_VS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS)
				&& ((VertexShaderStageStateManager*)this->setting)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_PS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS)
				&& ((PixelShaderStageStateManager*)this->setting)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_GS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS)
				&& ((GeometryShaderStageStateManager*)this->setting)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_HS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS)
				&& ((HullShaderStageStateManager*)this->setting)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_DS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS)
				&& ((DomainShaderStageStateManager*)this->setting)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_CS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS)
				&& ((ComputeShaderStageStateManager*)this->setting)->_Description.pShaderBytecode != nullptr;
		}
#pragma endregion

		void __OnInitialization(gObj<DeviceManager> manager) {
			this->manager = manager;
			Setup();
			__CurrentLoadingCSU = &__GlobalsCSU;
			__CurrentLoadingSamplers = &__GlobalsSamplers;
			Globals();
			__CurrentLoadingCSU = &__LocalsCSU;
			__CurrentLoadingSamplers = &__LocalsSamplers;
			Locals();
		}

		void __OnSet(ComputeManager* manager);

		void __OnDraw(ComputeManager* manager) {
			BindOnGPU(manager, __LocalsCSU, __GlobalsCSU.size() + __GlobalsSamplers.size());
		}

		void BindOnGPU(ComputeManager* manager, const list<SlotBinding> &list, int startRootParameter);

		// Creates the state object (compute or graphics)
		void CreatePSO() {

			if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE))
			{
				((RootSignatureStateManager*)setting)->SetRootSignature(rootSignature);
			}
			if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS))
			{
				RenderTargetFormatsStateManager* rtfsm = (RenderTargetFormatsStateManager*)setting;
				if (rtfsm->_Description.NumRenderTargets == 0)
					rtfsm->AllRenderTargets(8, DXGI_FORMAT_R8G8B8A8_UNORM);
			}
			if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT))
			{
				DepthStencilFormatStateManager* dsfsm = (DepthStencilFormatStateManager*)setting;
				dsfsm->_Description = DXGI_FORMAT_D32_FLOAT;
			}
			/*if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT))
			{
				DepthStencilFormatStateManager* dsf = (DepthStencilFormatStateManager*)setting;
				dsf->DepthStencilFormat(DXGI_FORMAT_UNKNOWN);
			}*/
			const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(*setting), setting };
			auto hr = manager->device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
			if (FAILED(hr)) {
				//auto hr = manager->device->GetDeviceRemovedReason();
				throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
			}
		}

		// Creates the root signature after closing
		void CreateRootSignature() {
			D3D12_ROOT_PARAMETER1 * parameters = new D3D12_ROOT_PARAMETER1[__GlobalsCSU.size() + __GlobalsSamplers.size() + __LocalsCSU.size() + __LocalsSamplers.size()];
			D3D12_DESCRIPTOR_RANGE1 * ranges = new D3D12_DESCRIPTOR_RANGE1[__GlobalsCSU.size() + __GlobalsSamplers.size() + __LocalsCSU.size() + __LocalsSamplers.size()];
			int index = 0;
			for (int i = 0; i < __GlobalsCSU.size(); i++)
			{
				D3D12_DESCRIPTOR_RANGE1 range = { };
				range.RangeType = __GlobalsCSU[i].type;
				range.BaseShaderRegister = __GlobalsCSU[i].slot;
				range.NumDescriptors = __GlobalsCSU[i].ptrToCount == nullptr ? 1 : -1;
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
				ranges[index] = range;

				D3D12_ROOT_PARAMETER1 p = { };
				p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				p.DescriptorTable.NumDescriptorRanges = 1;
				p.DescriptorTable.pDescriptorRanges = &ranges[index];
				p.ShaderVisibility = __GlobalsCSU[i].visibility;
				parameters[index] = p;

				index++;
			}

			for (int i = 0; i < __GlobalsSamplers.size(); i++)
			{
				D3D12_DESCRIPTOR_RANGE1 range = { };
				range.RangeType = __GlobalsSamplers[i].type;
				range.BaseShaderRegister = __GlobalsSamplers[i].slot;
				range.NumDescriptors = __GlobalsSamplers[i].ptrToCount == nullptr ? 1 : -1;
				range.RegisterSpace = 0;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
				ranges[index] = range;

				D3D12_ROOT_PARAMETER1 p = { };
				p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				p.DescriptorTable.NumDescriptorRanges = 1;
				p.DescriptorTable.pDescriptorRanges = &ranges[index];
				p.ShaderVisibility = __GlobalsSamplers[i].visibility;
				parameters[index] = p;

				index++;
			}

			for (int i = 0; i < __LocalsCSU.size(); i++)
			{
				D3D12_DESCRIPTOR_RANGE1 range = { };
				range.RangeType = __LocalsCSU[i].type;
				range.BaseShaderRegister = __LocalsCSU[i].slot;
				range.NumDescriptors = __LocalsCSU[i].ptrToCount == nullptr ? 1 : -1;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				range.RegisterSpace = 0;
				//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
				ranges[index] = range;

				D3D12_ROOT_PARAMETER1 p = { };
				p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				p.DescriptorTable.NumDescriptorRanges = 1;
				p.DescriptorTable.pDescriptorRanges = &ranges[index];
				p.ShaderVisibility = __LocalsCSU[i].visibility;
				parameters[index] = p;

				index++;
			}

			for (int i = 0; i < __LocalsSamplers.size(); i++)
			{
				D3D12_DESCRIPTOR_RANGE1 range = { };
				range.RangeType = __LocalsSamplers[i].type;
				range.BaseShaderRegister = __LocalsSamplers[i].slot;
				range.NumDescriptors = __LocalsSamplers[i].ptrToCount == nullptr ? 1 : -1;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				range.RegisterSpace = 0;
				//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
				ranges[index] = range;

				D3D12_ROOT_PARAMETER1 p = { };
				p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				p.DescriptorTable.NumDescriptorRanges = 1;
				p.DescriptorTable.pDescriptorRanges = &ranges[index];
				p.ShaderVisibility = __LocalsSamplers[i].visibility;
				parameters[index] = p;

				index++;
			}

			D3D12_ROOT_SIGNATURE_DESC1 desc;
			desc.pParameters = parameters;
			desc.NumParameters = index;
			desc.pStaticSamplers = Static_Samplers;
			desc.NumStaticSamplers = StaticSamplerMax;
			desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			if (!Using_VS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			if (!Using_PS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			if (!Using_GS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			if (!Using_HS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			if (!Using_DS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

			ID3DBlob *signatureBlob;
			ID3DBlob *signatureErrorBlob;

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC d = {};
			d.Desc_1_1 = desc;
			d.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;

			auto hr = D3D12SerializeVersionedRootSignature(&d, &signatureBlob, &signatureErrorBlob);

			if (hr != S_OK)
			{
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error serializing root signature");
			}

			hr = manager->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

			if (hr != S_OK)
			{
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error creating root signature");
			}

			delete[] parameters;
			delete[] ranges;
		}

	protected:

		PipelineBindings() :setting(new PipelineStateStreamDescription())
		{
			StreamBits = StreamTypeBits<PSS...>();
		}

		// Binds a constant buffer view
		void CBV(int slot, gObj<Buffer>& resource, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource , nullptr, space });
		}

		// Binds a shader resource view
		void SRV(int slot, gObj<Buffer>& resource, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
		}

		// Binds a shader resource view
		void SRV(int slot, gObj<Texture2D>& resource, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource , nullptr, space });
		}

		// Binds a shader resource view
		void SRV_Array(int startSlot, gObj<Texture2D>*& resources, int &count, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, startSlot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resources, &count, space });
		}

		// Binds a sampler object view
		void SMP(int slot, gObj<Sampler> &sampler, ShaderType type) {
			__CurrentLoadingSamplers->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, slot, D3D12_RESOURCE_DIMENSION_UNKNOWN, (void*)&sampler });
		}

		// Binds an unordered access view
		void UAV(int slot, gObj<Buffer> &resource, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
		}

		// Binds an unordered access view
		void UAV(int slot, gObj<Texture2D> &resource, ShaderType type, int space = 0) {
			__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource, nullptr, space });
		}

		// Binds a render target view
		void RTV(int slot, gObj<Texture2D> &resource) {
			RenderTargetMax = max(RenderTargetMax, slot + 1);
			RenderTargets[slot] = &resource;
		}

		// Binds a depth-stencil buffer view
		void DBV(gObj<Texture2D> &resource) {
			DepthBufferField = (gObj<Texture2D>*)&resource;
		}

		// Adds a static sampler to the root signature.
		void Static_SMP(int slot, Sampler &sampler, ShaderType type, int space = 0) {
			D3D12_STATIC_SAMPLER_DESC desc = { };
			desc.AddressU = sampler.AddressU;
			desc.AddressV = sampler.AddressV;
			desc.AddressW = sampler.AddressW;
			desc.BorderColor =
				!any(sampler.BorderColor - float4(0, 0, 0, 0)) ?
				D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
				!any(sampler.BorderColor - float4(0, 0, 0, 1)) ?
				D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK :
				D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			desc.ComparisonFunc = sampler.ComparisonFunc;
			desc.Filter = sampler.Filter;
			desc.MaxAnisotropy = sampler.MaxAnisotropy;
			desc.MaxLOD = sampler.MaxLOD;
			desc.MinLOD = sampler.MinLOD;
			desc.MipLODBias = sampler.MipLODBias;
			desc.RegisterSpace = space;
			desc.ShaderRegister = slot;
			desc.ShaderVisibility = (D3D12_SHADER_VISIBILITY)type;

			Static_Samplers[StaticSamplerMax] = desc;
			StaticSamplerMax++;
		}

		// When implemented by users, this method will setup the pipeline object after created
		// Use this method to specify how to load shaders and set other default pipeline settings
		virtual void Setup() = 0;

		// When implemented by users, this method will bind all globals (per frame) resources
		virtual void Globals() {};

		// When implemented by users, this method will bind all locals (per draw) resources
		virtual void Locals() {};

	public:
		struct PipelineStateStreamDescription : PSS...{
		} *setting;

		// When executed, the sublaying pipeline state object is built 
		// and can not be changed without opening again
		void Close() {
			CreateRootSignature();
			CreatePSO();
		}

		// Gets if this pipeline object is openned and can be setup.
		bool IsClosed() {
			return pso != nullptr;
		}

		// Opens this graphics pipeline object. Reseting pipeline state object and 
		// leaving this object ready to be setup.
		PipelineBindings* Open() {
			if (pso != nullptr) {
				//pso->Release();
				delete pso;
			}
			pso = nullptr;
			return this;
		}
	};

	struct GraphicsPipelineBindings : public PipelineBindings <
		DebugStateManager,
		VertexShaderStageStateManager,
		PixelShaderStageStateManager,
		DomainShaderStageStateManager,
		HullShaderStageStateManager,
		GeometryShaderStageStateManager,
		StreamOutputStateManager,
		BlendingStateManager,
		BlendSampleMaskStateManager,
		RasterizerStateManager,
		DepthStencilStateManager,
		InputLayoutStateManager,
		IndexBufferStripStateManager,
		PrimitiveTopologyStateManager,
		RenderTargetFormatsStateManager,
		DepthStencilFormatStateManager,
		MultisamplingStateManager,
		NodeMaskStateManager,
		RootSignatureStateManager
	> {
	};

	class ShowComplexityPipeline : public GraphicsPipelineBindings {
	public:
		// UAV to output the complexity
		gObj<Texture2D> complexity;

		// Render Target
		gObj<Texture2D> RenderTarget;

	protected:
		void Setup() {
			this->setting->VertexShader(ShaderLoader::FromMemory(cso_DrawScreen_VS));
			this->setting->PixelShader(ShaderLoader::FromMemory(cso_DrawComplexity_PS));
			this->setting->InputLayout({
					VertexElement(VertexElementType_Float, 2, "POSITION")
				});
		}

		void Globals()
		{
			RTV(0, RenderTarget);
			SRV(0, complexity, ShaderType_Pixel);
		}
	};

	class ShowTexturePipeline : public GraphicsPipelineBindings {
	public:
		// UAV to output the complexity
		gObj<Texture2D> Texture;
		// Render Target
		gObj<Texture2D> RenderTarget;

	protected:
		void Setup() {
			this->setting->VertexShader(ShaderLoader::FromMemory(cso_DrawScreen_VS));
			this->setting->PixelShader(ShaderLoader::FromMemory(cso_DrawScreen_PS));
			this->setting->InputLayout({
					VertexElement(VertexElementType_Float, 2, "POSITION")
				});
		}

		void Globals()
		{
			RTV(0, RenderTarget);
			SRV(0, Texture, ShaderType_Pixel);
		}
	};

#pragma endregion

}

#pragma endregion

#pragma region ca4GRTPipelineManager.h

namespace CA4G {

#pragma region Scene Manager

	class GeometriesOnGPU
	{
		friend GeometryCollection;
		friend InstanceCollection;
		friend ProceduralGeometryCollection;
		friend TriangleGeometryCollection;

		gObj<Buffer> bottomLevelAccDS;
		gObj<Buffer> scratchBottomLevelAccDS;
		WRAPPED_GPU_POINTER emulatedPtr;

		gObj<list<D3D12_RAYTRACING_GEOMETRY_DESC>> geometries;
	};

	// Represents a geometry collection can be used to compose scenes
	// Internally manage a bottom level acceleration structure
	// This structure can be lock/unlock for further modifications
	class GeometryCollection {
		friend TriangleGeometryCollection;
		friend ProceduralGeometryCollection;

		gObj<DeviceManager> manager;
		DXRManager* cmdList;

		GeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList) :
			manager(manager), cmdList(cmdList),
			creating(new Creating(this)) {
		}
		GeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList, gObj<GeometriesOnGPU> bakedGeometry) :
			manager(manager), cmdList(cmdList),
			creating(new Creating(this)) {
			this->isUpdating = true;
			this->currentGeometry = 0;
			this->updatingGeometry = bakedGeometry;
			this->geometries = bakedGeometry->geometries;
		}

	protected:
		gObj<list<D3D12_RAYTRACING_GEOMETRY_DESC>> geometries = new list<D3D12_RAYTRACING_GEOMETRY_DESC>();
		bool isUpdating = false;
		int currentGeometry;
		gObj<GeometriesOnGPU> updatingGeometry;

	public:

		void PrepareBuffer(gObj<Buffer> bufferForGeometry);

		class Creating {
			friend GeometryCollection;
			GeometryCollection* manager;
			Creating(GeometryCollection* manager) :manager(manager) {}
		public:
			gObj<GeometriesOnGPU> BakedGeometry(bool allowUpdates = false, bool preferFastTrace = true);
			gObj<GeometriesOnGPU> UpdatedGeometry();
		} *const creating;
	};

	// Represents a triangle-based geometry collection
	struct TriangleGeometryCollection : GeometryCollection {
		friend DXRManager;
	private:
		gObj<Buffer> boundVertices;
		gObj<Buffer> boundIndices;
		gObj<Buffer> boundTransforms;
		int currentVertexOffset = 0;
		DXGI_FORMAT currentVertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		TriangleGeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList) :
			GeometryCollection(manager, cmdList),
			loading(new Loader(this)), setting(new Setting(this)) {}

		TriangleGeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList, gObj<GeometriesOnGPU> bakedGeometry) :
			GeometryCollection(manager, cmdList, bakedGeometry),
			loading(new Loader(this)), setting(new Setting(this)) {
		}
	public:
		class Loader {
			friend TriangleGeometryCollection;
			TriangleGeometryCollection* manager;
			Loader(TriangleGeometryCollection* manager) :manager(manager) {}
		public:
			void Geometry(int startVertex, int count, int transformIndex = -1) {
				if (manager->isUpdating) {
					if (manager->currentGeometry >= manager->geometries->size())
						throw CA4GException("Can not change geometry count during updating");
					if (manager->geometries[manager->currentGeometry].Triangles.VertexCount != count)
						throw CA4GException("Can not change vertex count during updating");
					D3D12_RAYTRACING_GEOMETRY_DESC &desc = manager->geometries[manager->currentGeometry];
					desc.Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
					{
						manager->boundVertices->resource->GetGPUVirtualAddress() + startVertex * manager->boundVertices->Stride + manager->currentVertexOffset,
						manager->boundVertices->Stride
					};
					desc.Triangles.VertexFormat = manager->currentVertexFormat;
					if (transformIndex >= 0)
						desc.Triangles.Transform3x4 = manager->boundTransforms->resource->GetGPUVirtualAddress() + transformIndex * sizeof(float3x4);
					manager->currentGeometry++;
				}
				else {
					D3D12_RAYTRACING_GEOMETRY_DESC desc{ };
					desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE::D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
					desc.Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
					{
						manager->boundVertices->resource->GetGPUVirtualAddress() + startVertex * manager->boundVertices->Stride + manager->currentVertexOffset,
						manager->boundVertices->Stride
					};
					if (transformIndex >= 0)
						desc.Triangles.Transform3x4 = manager->boundTransforms->resource->GetGPUVirtualAddress() + transformIndex * sizeof(float3x4);
					desc.Triangles.VertexFormat = manager->currentVertexFormat;
					desc.Triangles.VertexCount = count;

					manager->geometries->add(desc);
				}
			}
		} *const loading;

		class Setting {
			friend TriangleGeometryCollection;
			TriangleGeometryCollection* manager;
			Setting(TriangleGeometryCollection* manager) :manager(manager) {}

			template<int N>
			void UpdateLayout(VertexElement(&vertexLayout)[N], int positionIndex = 0) {
				int offset = 0;
				D3D12_INPUT_ELEMENT_DESC d;
				int vertexElementIndex = 0;
				while (positionIndex >= 0)
				{
					int size;
					d = vertexLayout[vertexElementIndex].createDesc(offset, size);
					if (positionIndex == 0)
						break;
					offset += size;
					positionIndex--;
					vertexElementIndex++;
				}
				manager->currentVertexFormat = d.Format;
				manager->currentVertexOffset = offset;
			}

			void UpdateLayout(std::initializer_list<VertexElement> elements, int positionIndex = 0) {
				int offset = 0;
				D3D12_INPUT_ELEMENT_DESC d;
				int vertexElementIndex = 0;
				auto currentElement = elements.begin();
				while (positionIndex >= 0)
				{
					int size;
					d = currentElement->createDesc(offset, size);
					if (positionIndex == 0)
						break;
					offset += size;
					positionIndex--;
					vertexElementIndex++;
					currentElement++;
				}
				manager->currentVertexFormat = d.Format;
				manager->currentVertexOffset = offset;
			}

		public:
			void VertexBuffer(gObj<Buffer> vertices, std::initializer_list<VertexElement> elements, int positionIndex = 0) {
				UpdateLayout(elements, positionIndex);
				manager->boundVertices = vertices;
			}
			template<int N>
			void VertexBuffer(gObj<Buffer> vertices, VertexElement(&vertexLayout)[N], int positionIndex = 0) {
				UpdateLayout(vertexLayout, positionIndex);
				manager->boundVertices = vertices;
			}
			void IndexBuffer(gObj<Buffer> indices) {
				manager->boundIndices = indices;
			}
		} *const setting;
	};

	struct ProceduralGeometryCollection : GeometryCollection {
		friend DXRManager;
	private:
		gObj<Buffer> boundAABBs;
		ProceduralGeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList) :
			GeometryCollection(manager, cmdList),
			loading(new Loader(this)), setting(new Setting(this)) {}

		ProceduralGeometryCollection(gObj<DeviceManager> manager, DXRManager* cmdList, gObj<GeometriesOnGPU> bakedGeometries) :
			GeometryCollection(manager, cmdList, bakedGeometries),
			loading(new Loader(this)), setting(new Setting(this)) {
		}

	public:
		class Loader {
			friend ProceduralGeometryCollection;
			ProceduralGeometryCollection* manager;
			Loader(ProceduralGeometryCollection* manager) :manager(manager) {}
		public:
			void Geometry(int startBox, int count) {
				if (manager->isUpdating) {
					if (manager->geometries[manager->currentGeometry].AABBs.AABBCount != count)
						throw CA4GException("Can not change number of boxes during updates.");
					if (manager->currentGeometry >= manager->updatingGeometry->geometries->size())
						throw CA4GException("Can not change the number of geometries during updates.");
					manager->geometries[manager->currentGeometry].AABBs.AABBs = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
					{
						manager->boundAABBs->resource->GetGPUVirtualAddress() + startBox * manager->boundAABBs->Stride,
						manager->boundAABBs->Stride
					};
					manager->currentGeometry++;
				}
				else {
					D3D12_RAYTRACING_GEOMETRY_DESC desc{ };
					desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE::D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
					desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
					desc.AABBs.AABBCount = count;
					desc.AABBs.AABBs = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
					{
						manager->boundAABBs->resource->GetGPUVirtualAddress() + startBox * manager->boundAABBs->Stride,
						manager->boundAABBs->Stride
					};
					manager->geometries->add(desc);
				}
			}
		} *const loading;

		class Setting {
			friend ProceduralGeometryCollection;
			ProceduralGeometryCollection* manager;
			Setting(ProceduralGeometryCollection* manager) :manager(manager) {}

		public:
			void AABBs(gObj<Buffer> aabbs) {
				manager->boundAABBs = aabbs;
			}
		} *const setting;
	};

	class SceneOnGPU {
		friend InstanceCollection;
		friend IRTProgram;

		gObj<Buffer> topLevelAccDS;
		gObj<Buffer> scratchBuffer;
		gObj<Buffer> instancesBuffer;
		gObj<list<gObj<GeometriesOnGPU>>> usedGeometries;
		WRAPPED_GPU_POINTER topLevelAccFallbackPtr;

		gObj<list<D3D12_RAYTRACING_INSTANCE_DESC>> instances;
		gObj<list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC>> fallbackInstances;
	};

	// Represents a scene as a collection of instances
	class InstanceCollection {

		friend DXRManager;

		gObj<DeviceManager> manager;
		DXRManager* cmdList;

		gObj<list<gObj<GeometriesOnGPU>>> usedGeometries;
		gObj<list<D3D12_RAYTRACING_INSTANCE_DESC>> instances;
		gObj<list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC>> fallbackInstances;

		// updating
		gObj<SceneOnGPU> updatingScene = nullptr;
		bool isUpdating = false;
		int currentInstance;

		InstanceCollection(gObj<DeviceManager> manager, DXRManager* cmdList) :manager(manager), cmdList(cmdList),
			creating(new Creating(this)), loading(new Loading(this))
		{
			usedGeometries = new list<gObj<GeometriesOnGPU>>();
			instances = new list<D3D12_RAYTRACING_INSTANCE_DESC>();
			fallbackInstances = new list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC>();
		}

		InstanceCollection(gObj<DeviceManager> manager, DXRManager* cmdList, gObj<SceneOnGPU> scene) :manager(manager), cmdList(cmdList),
			creating(new Creating(this)), loading(new Loading(this))
		{
			usedGeometries = new list<gObj<GeometriesOnGPU>>();
			fallbackInstances = scene->fallbackInstances;
			instances = scene->instances;
			updatingScene = scene;
			isUpdating = true;
			currentInstance = 0;
		}
	public:
		class Loading {
			friend InstanceCollection;
			InstanceCollection* manager;
			Loading(InstanceCollection* manager) :manager(manager) {}
			void FillMat4x3(float(&dst)[3][4], float4x4 transform) {
				dst[0][0] = transform.M00;
				dst[0][1] = transform.M10;
				dst[0][2] = transform.M20;
				dst[0][3] = transform.M30;
				dst[1][0] = transform.M01;
				dst[1][1] = transform.M11;
				dst[1][2] = transform.M21;
				dst[1][3] = transform.M31;
				dst[2][0] = transform.M02;
				dst[2][1] = transform.M12;
				dst[2][2] = transform.M22;
				dst[2][3] = transform.M32;
			}

		public:
			void Instance(gObj<GeometriesOnGPU> geometries, UINT mask = 0xFF, float4x4 transform = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), UINT instanceID = INTSAFE_UINT_MAX);
		} *const loading;

		class Creating {
			friend InstanceCollection;
			InstanceCollection* manager;
			Creating(InstanceCollection* manager) :manager(manager) {}
		public:
			gObj<SceneOnGPU> BakedScene(bool allowUpdate = false, bool preferFastTrace = true);
			gObj<SceneOnGPU> UpdatedScene();

		} *const creating;
	};

#pragma endregion

#pragma region State Object Bindings

	class IStateBindings {
		friend DXRManager;

	protected:

		// When state setting is closed, the state object is created and stored.
		ID3D12StateObject *so = nullptr;
		// When state setting is closed, the fallback state object is created and stored.
		ID3D12RaytracingFallbackStateObject *fbso = nullptr;

		virtual void __OnSet(DXRManager* manager) = 0;

		virtual void __OnDraw(DXRManager* manager) = 0;

		virtual void __OnInitialization(DeviceManager* manager) = 0;
	};

	class DynamicStateBindings {

		friend RTPipelineManager;

		template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type> friend class DynamicStateBindingOf;

		D3D12_STATE_SUBOBJECT* dynamicStates;

		void AllocateStates(int length) {
			dynamicStates = new D3D12_STATE_SUBOBJECT[length];
		}

		template<typename D>
		D* GetState(int index) {
			return (D*)dynamicStates[index].pDesc;
		}

		template<typename D>
		void SetState(int index, D3D12_STATE_SUBOBJECT_TYPE type, D* state) {
			dynamicStates[index].Type = type;
			dynamicStates[index].pDesc = state;
		}

		/*template<typename D>
		D* GenericAddState(D3D12_STATE_SUBOBJECT_TYPE Type) {
			dynamicStates.add(D3D12_STATE_SUBOBJECT{ Type, new D{ } });
			return (D*)dynamicStates.last().pDesc;
		}

		template<typename D>
		D* GenericTrackState(D3D12_STATE_SUBOBJECT_TYPE Type) {
			for (int i = 0; i < dynamicStates.size(); i++)
				if (dynamicStates[i].Type == Type)
					return (D*)dynamicStates[i].pDesc;
			return nullptr;
		}

		template<typename D>
		D* GenericGetOrCreateState(D3D12_STATE_SUBOBJECT_TYPE Type) {
			auto state = GenericTrackState<D>(Type);
			if (state != nullptr)
				return state;
			return GenericAddState<D>(Type);
		}

		void ResetStates() {
			dynamicStates.reset();
		}*/
	public:
		DynamicStateBindings() {}
	};

	template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type>
	class DynamicStateBindingOf : public virtual DynamicStateBindings {
	protected:
		S* GetState(int index) {
			return GetState<S>(index);
		}

		S* GetAfterCreate(int index) {
			S* state = new S{ };
			SetState(index, Type, state);
			return state;
		}
	public:
	};


#pragma region State Managers

	// Represents a binding of a resource (or resource array) to a shader slot.
	struct SlotBinding {
		// Gets or sets the root parameter bound
		D3D12_ROOT_PARAMETER Root_Parameter;

		union {
			struct ConstantData {
				// Gets the pointer to a constant buffer in memory.
				void* ptrToConstant;
			} ConstantData;
			struct DescriptorData
			{
				// Determines the dimension of the bound resource
				D3D12_RESOURCE_DIMENSION Dimension;
				// Gets the pointer to a resource field (pointer to ResourceView)
				// or to the array of resources
				void* ptrToResourceViewArray;
				// Gets the pointer to the number of resources in array
				int* ptrToCount;
			} DescriptorData;
			struct SceneData {
				void* ptrToScene;
			} SceneData;
		};
	};


	class BindingsHandle {
		friend RTPipelineManager;
		friend DXRManager;
		friend IRTProgram;
		template<typename R> friend class RTProgram;

		DX_RootSignature rootSignature;
		bool isLocal;
		int rootSize;

		list<SlotBinding> csuBindings;
		list<SlotBinding> samplerBindings;

		D3D12_STATIC_SAMPLER_DESC Samplers[32];
		int Max_Sampler;

		void AddSampler(SlotBinding binding) {
			samplerBindings.add(binding);
		}
		void AddCSU(SlotBinding binding) {
			csuBindings.add(binding);
		}

	public:
		inline bool HasSomeBindings() {
			return csuBindings.size() + samplerBindings.size() > 0;
		}
	};

	class ProgramHandle {
		friend RTPipelineManager;
		friend DXRManager;
		template<typename C> friend class DXIL_Library;
		template<typename L> friend class RTProgram;
		friend IRTProgram;

		LPCWSTR shaderHandle;
		void* cachedShaderIdentifier;
	protected:
		ProgramHandle(LPCWSTR handle) :shaderHandle(handle) {}
	public:
		ProgramHandle() :shaderHandle(nullptr) {}
		ProgramHandle(const ProgramHandle &other) {
			this->shaderHandle = other.shaderHandle;
		}
		inline bool IsNull() { return shaderHandle == nullptr; }
	};

	class MissHandle : public ProgramHandle {
		friend RTPipelineManager;
		friend DXRManager;
		template<typename C> friend class DXIL_Library;

		MissHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) { }
	public:
		MissHandle() : ProgramHandle() {}
		MissHandle(const MissHandle &other) : ProgramHandle(other) { }
	};

	class RayGenerationHandle : public ProgramHandle {
		friend RTPipelineManager;
		friend DXRManager;
		template<typename C> friend class DXIL_Library;

		RayGenerationHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}

	public:
		RayGenerationHandle() : ProgramHandle() { }
		RayGenerationHandle(const RayGenerationHandle &other) : ProgramHandle(other) { }
	};

	class AnyHitHandle : public ProgramHandle {
		friend RTPipelineManager;
		template<typename C> friend class DXIL_Library;

		AnyHitHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}

	public:
		AnyHitHandle() : ProgramHandle() { }
		AnyHitHandle(const AnyHitHandle &other) : ProgramHandle(other) { }
	};

	class ClosestHitHandle : public ProgramHandle {
		friend RTPipelineManager;
		template<typename C> friend class DXIL_Library;

		ClosestHitHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}

	public:
		ClosestHitHandle() : ProgramHandle() { }
		ClosestHitHandle(const ClosestHitHandle &other) : ProgramHandle(other) { }
	};

	class IntersectionHandle : public ProgramHandle {
		friend RTPipelineManager;
		template<typename C> friend class DXIL_Library;

		IntersectionHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}

	public:
		IntersectionHandle() : ProgramHandle() { }
		IntersectionHandle(const IntersectionHandle &other) : ProgramHandle(other) { }
	};

	class HitGroupHandle : public ProgramHandle {
		friend RTPipelineManager;
		friend DXRManager;
		template<typename C> friend class RTProgram;

		static LPCWSTR CreateAutogeneratedName() {
			static int ID = 0;

			wchar_t* label = new wchar_t[100];
			label[0] = 0;
			wcscat_s(label, 100, TEXT("HITGROUP"));
			wsprintf(&label[8], L"%d",
				ID);

			ID++;

			return label;
		}

		gObj<AnyHitHandle> anyHit;
		gObj<ClosestHitHandle> closestHit;
		gObj<IntersectionHandle> intersection;

		HitGroupHandle(gObj<ClosestHitHandle> closestHit, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection)
			: ProgramHandle(CreateAutogeneratedName()), anyHit(anyHit), closestHit(closestHit), intersection(intersection) {}
	public:
		HitGroupHandle() :ProgramHandle() {}
		HitGroupHandle(const HitGroupHandle &other) :ProgramHandle(other) {
			this->anyHit = other.anyHit;
			this->closestHit = other.closestHit;
			this->intersection = other.intersection;
		}
	};


	struct GlobalRootSignatureManager : public DynamicStateBindingOf<D3D12_GLOBAL_ROOT_SIGNATURE, D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE> {
		void SetGlobalRootSignature(int index, ID3D12RootSignature *rootSignature) {
			auto state = GetAfterCreate(index);
			state->pGlobalRootSignature = rootSignature;
		}
	};

	struct LocalRootSignatureManager : public DynamicStateBindingOf<D3D12_LOCAL_ROOT_SIGNATURE, D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE> {
		void SetLocalRootSignature(int index, ID3D12RootSignature *rootSignature) {
			auto state = GetAfterCreate(index);
			state->pLocalRootSignature = rootSignature;
		}
	};

	struct HitGroupManager : public virtual DynamicStateBindingOf<D3D12_HIT_GROUP_DESC, D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP> {
		void SetTriangleHitGroup(int index, LPCWSTR exportName, LPCWSTR anyHit, LPCWSTR closestHit) {
			auto hg = GetAfterCreate(index);
			hg->AnyHitShaderImport = anyHit;
			hg->ClosestHitShaderImport = closestHit;
			hg->IntersectionShaderImport = nullptr;
			hg->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			hg->HitGroupExport = exportName;
		}
		void SetProceduralGeometryHitGroup(int index, LPCWSTR exportName, LPCWSTR anyHit, LPCWSTR closestHit, LPCWSTR intersection) {
			auto hg = GetAfterCreate(index);
			hg->AnyHitShaderImport = anyHit;
			hg->ClosestHitShaderImport = closestHit;
			hg->IntersectionShaderImport = intersection;
			hg->Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
			hg->HitGroupExport = exportName;
		}
	};

#pragma endregion

	struct DXILManager : public virtual DynamicStateBindingOf<D3D12_DXIL_LIBRARY_DESC, D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY> {
		void SetDXIL(int index, D3D12_SHADER_BYTECODE bytecode, list<D3D12_EXPORT_DESC> &exports) {
			auto dxil = GetAfterCreate(index);
			dxil->DXILLibrary = bytecode;
			dxil->NumExports = exports.size();
			dxil->pExports = &exports.first();
		}
	};

	struct RTShaderConfig : public virtual DynamicStateBindingOf<D3D12_RAYTRACING_SHADER_CONFIG, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG>
	{
		void SetRTSizes(int index, int maxAttributeSize, int maxPayloadSize) {
			auto state = GetAfterCreate(index);
			state->MaxAttributeSizeInBytes = maxAttributeSize;
			state->MaxPayloadSizeInBytes = maxPayloadSize;
		}
	};

	struct RTPipelineConfig : public virtual DynamicStateBindingOf<D3D12_RAYTRACING_PIPELINE_CONFIG, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG>
	{
		void SetMaxRTRecursion(int index, int maxRecursion) {
			auto state = GetAfterCreate(index);
			state->MaxTraceRecursionDepth = maxRecursion;
		}
	};

	struct ExportsManager : public virtual DynamicStateBindingOf<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION, D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION>
	{
		void SetExportsAssociations(int index, D3D12_STATE_SUBOBJECT* ptrToSubobject, const list<LPCWSTR> &exports) {
			auto state = GetAfterCreate(index);
			state->pSubobjectToAssociate = ptrToSubobject;
			state->NumExports = exports.size();
			state->pExports = &exports.first();
		}
	};

	// Gets the basic data of a dxil library
	struct IDXIL_Library {
		template<typename C> friend class DXIL_Library;
		friend RTPipelineManager;
	private:
		D3D12_SHADER_BYTECODE bytecode;
		list<D3D12_EXPORT_DESC> exports;
	};

	// Gets a base definition a library
	template<typename C>
	struct DXIL_Library : public IDXIL_Library {
		friend DXIL_Library;
		friend RTPipelineManager;
	private:

		C* context;
		void __OnInitialization(RTPipelineManager* context) {
			this->context = (C*)context;
			Setup();
		}

	protected:
		DXIL_Library() : loading(new Loading(this)) {
		}

		virtual void Setup() = 0;

		class Loading {
			DXIL_Library *manager;
		public:
			Loading(DXIL_Library* manager) :manager(manager) {}

			void DXIL(D3D12_SHADER_BYTECODE bytecode) {
				manager->bytecode = bytecode;
			}
			void Shader(gObj<RayGenerationHandle> &handle, LPCWSTR shader) {
				manager->exports.add({ shader });
				handle = new RayGenerationHandle(shader);
			}
			void Shader(gObj<MissHandle> &handle, LPCWSTR shader) {
				manager->exports.add({ shader });
				handle = new MissHandle(shader);
			}
			void Shader(gObj<AnyHitHandle> &handle, LPCWSTR shader) {
				manager->exports.add({ shader });
				handle = new AnyHitHandle(shader);
			}
			void Shader(gObj<ClosestHitHandle> &handle, LPCWSTR shader) {
				manager->exports.add({ shader });
				handle = new ClosestHitHandle(shader);
			}
			void Shader(gObj<IntersectionHandle> &handle, LPCWSTR shader) {
				manager->exports.add({ shader });
				handle = new IntersectionHandle(shader);
			}
		} *const loading;
	public:
		inline C* Context() { return context; }
	};

	// Interface of every raytracing program
	class IRTProgram {
		template<typename L> friend class RTProgram;
		friend RTPipelineManager;
		friend DXRManager;

		//! Gets the global bindings for this rt program
		gObj<BindingsHandle> globals;
		//! Gets the locals raygen bindings
		gObj<BindingsHandle> raygen_locals;
		//! Gets the locals miss bindings
		gObj<BindingsHandle> miss_locals;
		//! Gets the locals hitgroup bindings
		gObj<BindingsHandle> hitGroup_locals;
		// Gets the list of all hit groups created in this rt program
		list<gObj<HitGroupHandle>> hitGroups;
		// Gets the list of all associations between shaders and global bindings
		list<LPCWSTR> associationsToGlobal;
		// Gets the list of all associations between shaders and raygen local bindings
		list<LPCWSTR> associationsToRayGenLocals;
		// Gets the list of all associations between shaders and miss local bindings
		list<LPCWSTR> associationsToMissLocals;
		// Gets the list of all associations between shaders and hitgroup local bindings
		list<LPCWSTR> associationsToHitGroupLocals;

		list<gObj<ProgramHandle>> loadedShaderPrograms;

		// Shader table for ray generation shader
		gObj<Buffer> raygen_shaderTable;
		// Shader table for all miss shaders
		gObj<Buffer> miss_shaderTable;
		// Shader table for all hitgroup entries
		gObj<Buffer> group_shaderTable;
		// Gets the attribute size in bytes for this program (normally 2 floats)
		int AttributesSize = 2 * 4;
		// Gets the ray payload size in bytes for this program (normally 3 floats)
		int PayloadSize = 3 * 4;
		// Gets the stack size required for this program
		int StackSize = 1;

		// Gets the maximum number of hit groups that will be setup before any 
		// single dispatch rays
		int MaxGroups = 1024*1024;
		// Gets the maximum number of miss programs that will be setup before any
		// single dispatch rays
		int MaxMiss = 10;
		gObj<DeviceManager> manager;

		void UpdateRayGenLocals(DXRManager* cmdList, gObj<RayGenerationHandle> shader);
		void UpdateMissLocals(DXRManager* cmdList, gObj<MissHandle> shader, int index);
		void UpdateHitGroupLocals(DXRManager* cmdList, gObj<HitGroupHandle> shader, int index);

		void BindOnGPU(DXRManager* manager, gObj<BindingsHandle> globals);

		void BindLocalsOnShaderTable(gObj<BindingsHandle> locals, byte* shaderRecordData);
	};

	// Represents a raytracing program with shader tables and local bindings management
	template<typename R>
	class RTProgram : IRTProgram {
		friend RTPipelineManager;
		friend DXRManager;
		friend gObj<RTProgram<R>>;

		R* _rt_manager;

		void __OnInitialization(gObj<DeviceManager> manager, RTPipelineManager* rtManager);

		// Creates the root signature after closing
		DX_RootSignature CreateRootSignature(gObj<BindingsHandle> bindings, int &size);

		gObj<BindingsHandle> __CurrentBindings;

		// Creates a bindings handle object for a specific method
		// With bindings lists and root signature.
		void BuildRootSignatureForCurrentBindings() {
			auto handle = __CurrentBindings;
			int size = 0;
			handle->rootSignature = CreateRootSignature(handle, size);
			handle->rootSize = size;
		}

	protected:

		RTProgram() : loading(new Loader(this)), creating(new Creating(this)), setting(new Setter(this)) {
		}
		~RTProgram() {}

#pragma region Binding Methods
		// Binds a constant buffer view
		void CBV(int slot, gObj<Buffer>& const resource, int space = 0) {

			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			p.Descriptor.RegisterSpace = space;
			p.Descriptor.ShaderRegister = slot;

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}

		// Binds a constant buffer view
		template<typename D>
		void CBV(int slot, D &data, int space = 0) {
			int size = ((sizeof(D) - 1) / 4) + 1;

			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			p.Constants.Num32BitValues = size;
			p.Constants.RegisterSpace = space;
			p.Constants.ShaderRegister = slot;

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.ConstantData.ptrToConstant = (void*)&data;
			__CurrentBindings->csuBindings.add(b);
		}

		list<D3D12_DESCRIPTOR_RANGE> ranges;

		// Binds a top level acceleration data structure
		void ADS(int slot, gObj<SceneOnGPU>& const resource, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			p.Descriptor.ShaderRegister = slot;
			p.Descriptor.RegisterSpace = space;

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.SceneData.ptrToScene = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}


		// Binds a shader resource view
		void SRV(int slot, gObj<Buffer>& const resource, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			p.DescriptorTable.NumDescriptorRanges = 1;
			D3D12_DESCRIPTOR_RANGE range = { };
			range.BaseShaderRegister = slot;
			range.NumDescriptors = 1;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.RegisterSpace = space;

			ranges.add(range);
			p.DescriptorTable.pDescriptorRanges = &ranges.last();

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}

		// Binds a shader resource view
		void SRV(int slot, gObj<Texture2D>& const resource, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			p.DescriptorTable.NumDescriptorRanges = 1;
			D3D12_DESCRIPTOR_RANGE range = { };
			range.BaseShaderRegister = slot;
			range.NumDescriptors = 1;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.RegisterSpace = space;

			ranges.add(range);
			p.DescriptorTable.pDescriptorRanges = &ranges.last();

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}

		// Binds a shader resource view
		void SRV_Array(int startSlot, gObj<Texture2D>*& const resources, int &count, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			p.DescriptorTable.NumDescriptorRanges = 1;
			D3D12_DESCRIPTOR_RANGE range = { };
			range.BaseShaderRegister = startSlot;
			range.NumDescriptors = -1;// undefined this moment
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.RegisterSpace = space;

			ranges.add(range);
			p.DescriptorTable.pDescriptorRanges = &ranges.last();

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resources;
			b.DescriptorData.ptrToCount = &count;
			__CurrentBindings->csuBindings.add(b);
		}

		// Binds an unordered access view
		void UAV(int slot, gObj<Buffer> const &resource, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			p.DescriptorTable.NumDescriptorRanges = 1;
			D3D12_DESCRIPTOR_RANGE range = { };
			range.BaseShaderRegister = slot;
			range.NumDescriptors = 1;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			range.RegisterSpace = space;

			ranges.add(range);
			p.DescriptorTable.pDescriptorRanges = &ranges.last();

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}

		// Binds an unordered access view
		void UAV(int slot, gObj<Texture2D> const &resource, int space = 0) {
			D3D12_ROOT_PARAMETER p = { };
			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			p.DescriptorTable.NumDescriptorRanges = 1;
			D3D12_DESCRIPTOR_RANGE range = { };
			range.BaseShaderRegister = slot;
			range.NumDescriptors = 1;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			range.RegisterSpace = space;

			ranges.add(range);
			p.DescriptorTable.pDescriptorRanges = &ranges.last();

			SlotBinding b{ };
			b.Root_Parameter = p;
			b.DescriptorData.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
			__CurrentBindings->csuBindings.add(b);
		}

		// Adds a static sampler to the root signature.
		void Static_SMP(int slot, const Sampler &sampler, int space = 0) {
			D3D12_STATIC_SAMPLER_DESC desc = { };
			desc.AddressU = sampler.AddressU;
			desc.AddressV = sampler.AddressV;
			desc.AddressW = sampler.AddressW;
			desc.BorderColor =
				!any(sampler.BorderColor - float4(0, 0, 0, 0)) ?
				D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
				!any(sampler.BorderColor - float4(0, 0, 0, 1)) ?
				D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK :
				D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			desc.ComparisonFunc = sampler.ComparisonFunc;
			desc.Filter = sampler.Filter;
			desc.MaxAnisotropy = sampler.MaxAnisotropy;
			desc.MaxLOD = sampler.MaxLOD;
			desc.MinLOD = sampler.MinLOD;
			desc.MipLODBias = sampler.MipLODBias;
			desc.RegisterSpace = space;
			desc.ShaderRegister = slot;
			desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

			__CurrentBindings->Samplers[slot] = desc;
			__CurrentBindings->Max_Sampler = max(slot+1, __CurrentBindings->Max_Sampler);
		}

#pragma endregion

		class Loader {
			RTProgram* manager;
		public:
			Loader(RTProgram* manager) :manager(manager) { }

			void Shader(gObj<RayGenerationHandle> handle) {
				manager->associationsToGlobal.add(handle->shaderHandle);
				manager->associationsToRayGenLocals.add(handle->shaderHandle);
				manager->loadedShaderPrograms.add(handle);

			}

			void Shader(gObj<MissHandle> handle) {
				manager->associationsToGlobal.add(handle->shaderHandle);
				manager->associationsToMissLocals.add(handle->shaderHandle);
				manager->loadedShaderPrograms.add(handle);
			}

		} *const loading;

		class Creating {
			RTProgram *manager;
		public:
			Creating(RTProgram *manager) : manager(manager) {
			}

			void HitGroup(gObj<HitGroupHandle> &handle, gObj<ClosestHitHandle> closest, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection) {
				handle = new HitGroupHandle(closest, anyHit, intersection);
				manager->hitGroups.add(handle);
				manager->associationsToGlobal.add(handle->shaderHandle);
				manager->associationsToHitGroupLocals.add(handle->shaderHandle);

				if (!closest.isNull() && !closest->IsNull())
				{
					manager->associationsToGlobal.add(closest->shaderHandle);
					manager->associationsToHitGroupLocals.add(closest->shaderHandle);
				}
				if (!anyHit.isNull() && !anyHit->IsNull())
				{
					manager->associationsToGlobal.add(anyHit->shaderHandle);
					manager->associationsToHitGroupLocals.add(anyHit->shaderHandle);
				}
				if (!intersection.isNull() && !intersection->IsNull())
				{
					manager->associationsToGlobal.add(intersection->shaderHandle);
					manager->associationsToHitGroupLocals.add(intersection->shaderHandle);
				}
				manager->loadedShaderPrograms.add(handle);
			}
		} *const creating;

		class Setter {
			RTProgram *manager;
		public:
			Setter(RTProgram *manager) :manager(manager) {}
			void Payload(int sizeInBytes) {
				manager->PayloadSize = sizeInBytes;
			}
			void MaxHitGroupIndex(int index) {
				manager->MaxGroups = index + 1;
			}
			void StackSize(int maxDeep) {
				manager->StackSize = maxDeep;
			}
			void Attribute(int sizeInBytes) {
				manager->AttributesSize = sizeInBytes;
			}
		} *const setting;

		inline R* Context() { return _rt_manager; }

		virtual void Setup() = 0;

		virtual void Globals() { }

		virtual void RayGeneration_Locals() { }

		virtual void Miss_Locals() { }

		virtual void HitGroup_Locals() { }
	};

	struct RTPipelineManager : public virtual DynamicStateBindings,
		DXILManager,
		GlobalRootSignatureManager,
		LocalRootSignatureManager,
		HitGroupManager,
		RTShaderConfig,
		RTPipelineConfig,
		ExportsManager
	{
		friend DXRManager;
		friend Loading;
		friend IRTProgram;
	private:
		list<gObj<IDXIL_Library>> loadedLibraries;
		list<gObj<IRTProgram>> loadedPrograms;
		gObj<DeviceManager> manager;

		DX_FallbackStateObject fbso;
		DX_StateObject so;

		// Will be executed when this pipeline manager were loaded
		void __OnInitialization(gObj<DeviceManager> manager) {
			this->manager = manager;
			Setup();
		}

		void Close();

	public:
		D3D12_STATE_SUBOBJECT_TYPE GetStateType(int index) {
			return dynamicStates[index].Type;
		}

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* GetAsExports(int index) {
			return (D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*)dynamicStates[index].pDesc;
		}

		D3D12_DXIL_LIBRARY_DESC* GetAsDXIL(int index) {
			return (D3D12_DXIL_LIBRARY_DESC*)dynamicStates[index].pDesc;
		}

		D3D12_GLOBAL_ROOT_SIGNATURE* GetAsGlobalRootSignature(int index) {
			return (D3D12_GLOBAL_ROOT_SIGNATURE*)dynamicStates[index].pDesc;
		}

		D3D12_LOCAL_ROOT_SIGNATURE* GetAsLocalRootSignature(int index) {
			return (D3D12_LOCAL_ROOT_SIGNATURE*)dynamicStates[index].pDesc;
		}
	private:
		// Called when a pipeline is active into the pipeline
		void __OnSet(DXRManager* cmdManager);

	protected:


		class Loader {
			RTPipelineManager *manager;
		public:
			Loader(RTPipelineManager *manager) :manager(manager) {
			}

			template<typename P>
			void Program(gObj<P> &program) {
				program = new P();
				program->__OnInitialization(this->manager->manager, this->manager);
				manager->loadedPrograms.add(program);
			}

			template<typename L>
			void Library(gObj<L> &library) {
				library = new L();
				library->__OnInitialization(this->manager);
				manager->loadedLibraries.add(library);
			}
		} *const loading;

		virtual void Setup() = 0;

		RTPipelineManager() : loading(new Loader(this))
		{
		}
	};

#pragma endregion

}

#pragma endregion

#pragma region ca4GDeviceManager.h

namespace CA4G {

	// Wrapper to a DX 12 device. Exposes functionalities through Creating, Loading, and Perform interface.
	class DeviceManager {
		friend Presenter;
		friend ResourceWrapper;
		friend ResourceView;
		friend Creating;
		friend CommandListManager;
		template<typename ...A> friend class PipelineBindings;
		friend GraphicsPipelineBindings;
		friend ComputeManager;
		friend GraphicsManager;
		friend DXRManager;
		friend GPUScheduler;
		friend RTPipelineManager;
		friend class IRTProgram;
		template<typename R> friend class RTProgram;
		friend GeometryCollection;
		friend InstanceCollection;

		gObj<GPUScheduler> Scheduler;
		gObj<Texture2D> BackBuffer;
		gObj<CountEvent> counting;
		gObj<DescriptorsManager> descriptors;
		// Device object used to manage DX functionalities
		// Supported DXR interface here
		DX_Device device;

		// Fallback device object used to manage DXR functionalities when no GPU support.
		DX_FallbackDevice fallbackDevice;

		gObj<Buffer> __NullBuffer = nullptr;
		gObj<Texture2D> __NullTexture2D = nullptr;

		DeviceManager(DX_Device device, int buffers, bool useFrameBuffer, bool isWarpDevice);

		WRAPPED_GPU_POINTER CreateFallbackWrappedPointer(gObj<Buffer> resource, UINT bufferNumElements);

	public:
		Creating * const creating;
		Loading * const loading;

		// Gets or sets the tag used for next created process
		int Tag;

		inline gObj<Texture2D> getBackBuffer() { return BackBuffer; }

		~DeviceManager();

		// Performs a copying limited task synchronously.
		template<typename T>
		inline void Perform(CallableMember<T, CopyingManager> process) {
			process.TagID = Tag;
			Scheduler->Enqueue(&process);
		}
		template<typename T>
		// Performs a compute limited task synchronously.
		inline void Perform(CallableMember<T, ComputeManager> process) {
			process.TagID = Tag;
			Scheduler->Enqueue(&process);
		}
		// Performs a general graphics task synchronously.
		template<typename T>
		inline void Perform(CallableMember<T, GraphicsManager> process) {
			process.TagID = Tag;
			Scheduler->Enqueue(&process);
		}
		// Performs a general rtx task synchronously.
		template<typename T>
		inline void Perform(CallableMember<T, DXRManager> process) {
			process.TagID = Tag;
			Scheduler->Enqueue(&process);
		}

		// Performs a copying limited task asynchronously.
		template<typename T>
		inline void PerformAsync(CallableMember<T, CopyingManager>* process) {
			process->TagID = Tag;
			Scheduler->EnqueueAsync(process);
		}
		// Performs a compute limited task asynchronously.
		template<typename T>
		inline void PerformAsync(CallableMember<T, ComputeManager>* process) {
			process->TagID = Tag;
			Scheduler->EnqueueAsync(process);
		}
		// Performs a general graphics task asynchronously.
		template<typename T>
		inline void PerformAsync(CallableMember<T, GraphicsManager>* process) {
			process->TagID = Tag;
			Scheduler->EnqueueAsync(process);
		}
		// Performs a general rtx task asynchronously.
		template<typename T>
		inline void PerformAsync(CallableMember<T, DXRManager>* process) {
			process->TagID = Tag;
			Scheduler->EnqueueAsync(process);
		}

		// Ask for some engines to execute all pendings command list, and returns the mask of really active engines.
		inline int Flush(int engine_mask = ENGINE_MASK_ALL) { return Scheduler->Flush(engine_mask); }

		// Sends a signal throught the specific engines and returns the Signal object with all fence values
		inline Signal SendSignal(int engine_mask) {
			return Scheduler->SendSignal(engine_mask);
		}

		// Allows the CPU to synchronizes with all pending executed command list to be finished at the GPU when some signal were achieved.
		inline void WaitFor(const Signal &s) { Scheduler->WaitFor(s); }
	};

	// Represents a creation module to create resources
	class Creating {
		friend DeviceManager;
		friend CommandListManager;
		gObj<DeviceManager> manager;

		// Heap properties for default heap resources
		D3D12_HEAP_PROPERTIES defaultProp;
		// Heap properties for uploading heap resources
		D3D12_HEAP_PROPERTIES uploadProp;
		// Heap properties for downloading heap resources
		D3D12_HEAP_PROPERTIES downloadProp;

		int MaxMipsFor(int dimension) {
			int mips = 0;
			while (dimension > 0)
			{
				mips++;
				dimension >>= 1;
			}
			return mips;
		}

		// Tool function to create a wrapper from a created resource.
		gObj<ResourceWrapper> CreateResourceAndWrap(const D3D12_RESOURCE_DESC &desc, D3D12_RESOURCE_STATES state, CPU_ACCESS cpuAccess = CPU_ACCESS_NONE, D3D12_CLEAR_VALUE* clearDefault = nullptr) {

			D3D12_RESOURCE_DESC finalDesc = desc;
			if (cpuAccess != CPU_ACCESS_NONE) {
				auto size = GetRequiredIntermediateSize(manager->device, desc);
				finalDesc = { };
				finalDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				finalDesc.Format = DXGI_FORMAT_UNKNOWN;
				finalDesc.Width = size;
				finalDesc.Height = 1;
				finalDesc.DepthOrArraySize = 1;
				finalDesc.MipLevels = 1;
				finalDesc.SampleDesc.Count = 1;
				finalDesc.SampleDesc.Quality = 0;
				finalDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				finalDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			}

			DX_Resource resource;
			auto hr = manager->device->CreateCommittedResource(
				cpuAccess == CPU_ACCESS_NONE ? &defaultProp : cpuAccess == CPU_WRITE_GPU_READ ? &uploadProp : &downloadProp, D3D12_HEAP_FLAG_NONE, &finalDesc, state, clearDefault,
				IID_PPV_ARGS(&resource));

			if (FAILED(hr))
			{
				auto _hr = manager->device->GetDeviceRemovedReason();
				throw CA4GException::FromError(CA4G_Errors_RunOutOfMemory, nullptr, _hr);
			}
			return new ResourceWrapper(manager, desc, resource, state);
		}

		Creating(gObj<DeviceManager> manager) :manager(manager) {
			defaultProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			defaultProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			defaultProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			defaultProp.VisibleNodeMask = 1;
			defaultProp.CreationNodeMask = 1;

			uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
			uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			uploadProp.VisibleNodeMask = 1;
			uploadProp.CreationNodeMask = 1;

			downloadProp.Type = D3D12_HEAP_TYPE_READBACK;
			downloadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			downloadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			downloadProp.VisibleNodeMask = 1;
			downloadProp.CreationNodeMask = 1;
		}

	public:

		// Creates a shader table buffer
		gObj<Buffer> ShaderTable(D3D12_RESOURCE_STATES state, int stride, int count = 1, CPU_ACCESS cpuAccess = CPU_ACCESS::CPU_WRITE_GPU_READ, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {

			stride = (stride + (D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1)) & ~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);

			D3D12_RESOURCE_DESC desc = { };
			desc.Width = count * stride;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Flags = flags;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Height = 1;
			desc.Alignment = 0;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SampleDesc = { 1, 0 };

			return new Buffer(CreateResourceAndWrap(desc, state, cpuAccess), stride);
		}

		// Creates a generic buffer
		template <typename T>
		gObj<Buffer> GenericBuffer(D3D12_RESOURCE_STATES state, int count = 1, CPU_ACCESS cpuAccess = CPU_ACCESS_NONE, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
			D3D12_RESOURCE_DESC desc = { };
			desc.Width = count * sizeof(T);
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Flags = flags;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Height = 1;
			desc.Alignment = 0;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SampleDesc = { 1, 0 };

			return new Buffer(CreateResourceAndWrap(desc, state, cpuAccess), sizeof(T));
		}

	

		// Creates a 2D texture of specific element type.
		// Use int, float, unsigned int, float[2,3,4], int[2,3,4]
		template <class T>
		gObj<Texture2D> DrawableTexture2D(int width, int height, int mips = 1, int slices = 1, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) {
			D3D12_RESOURCE_DESC d;
			ZeroMemory(&d, sizeof(D3D12_RESOURCE_DESC));
			d.Width = width;
			d.Height = height;
			d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			d.DepthOrArraySize = slices;
			d.MipLevels = min(mips, MaxMipsFor(min(width, height)));
			d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			d.SampleDesc.Count = 1;
			d.SampleDesc.Quality = 0;
			d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			d.Format = Formats<T>::Value;
			D3D12_CLEAR_VALUE clearing = { };
			clearing.Format = d.Format;
			return new Texture2D(CreateResourceAndWrap(d, state, CPU_ACCESS_NONE, &clearing));
		}

		// Creates a 2D texture for depth buffer purpose (32-bit float format).
		gObj<Texture2D> DepthBuffer(int width, int height) {
			D3D12_RESOURCE_DESC d;
			ZeroMemory(&d, sizeof(D3D12_RESOURCE_DESC));
			d.Width = width;
			d.Height = height;
			d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			d.DepthOrArraySize = 1;
			d.MipLevels = 1;
			d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			d.SampleDesc.Count = 1;
			d.SampleDesc.Quality = 0;
			d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			d.Format = DXGI_FORMAT_D32_FLOAT;
			D3D12_CLEAR_VALUE clearing = { };
			clearing.Format = d.Format;
			clearing.DepthStencil.Depth = 1.0f;
			return new Texture2D(CreateResourceAndWrap(d, D3D12_RESOURCE_STATE_DEPTH_WRITE, CPU_ACCESS_NONE, &clearing));
		}

		// Creates a buffer for constants using specifc struct type for size.
		// This buffer is created with CPU_WRITE_GPU_READ accessibility in order to allow
		// an efficient per frame one CPU write and one GPU read optimization. If your constant buffer
		// will be uploaded less frequently you can create a generic buffer instead.
		template<typename T>
		gObj<Buffer> ConstantBuffer() {

			D3D12_RESOURCE_DESC desc = { };
			desc.Width = (sizeof(T) + 255)&(~255);
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Height = 1;
			desc.Alignment = 0;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.SampleDesc = { 1, 0 };

			return new Buffer(CreateResourceAndWrap(desc, D3D12_RESOURCE_STATE_GENERIC_READ, CPU_WRITE_GPU_READ), sizeof(T));
		}

		// Creates a buffer for vertex storing purposes of specific type.
		// This buffer is prepared to be written once and read by gpu every frame,
		// so, it is created using CPU_ACCESS_NONE flag. You must to upload the vertex buffer
		// at the start of the application.
		template<typename T>
		gObj<Buffer> VertexBuffer(int count) {
			return GenericBuffer<T>(D3D12_RESOURCE_STATE_COPY_DEST, count);
		}

		// Creates a buffer for indices storing purposes of specific type.
		// This buffer is prepared to be written once and read by gpu every frame,
		// so, it is created using CPU_ACCESS_NONE flag. You must to upload the index buffer
		// at the start of the application.
		template<typename T>
		gObj<Buffer> IndexBuffer(int count) {
			return GenericBuffer<T>(D3D12_RESOURCE_STATE_COPY_DEST, count);
		}

		// Creates a buffer for structured buffer purposes of specific type.
		// This resource is treated efficiently on the gpu and can be written at the beggining
		// using the uploading version.
		template<typename T>
		gObj<Buffer> StructuredBuffer(int count) {
			return GenericBuffer<T>(D3D12_RESOURCE_STATE_COPY_DEST, count);
		}

		// Creates a buffer for RW structured buffer purposes of specific type.
		// This resource is treated efficiently on the gpu and can be written at the beggining
		// using the uploading version.
		template<typename T>
		gObj<Buffer> RWStructuredBuffer(int count) {
			return GenericBuffer<T>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, count, CPU_ACCESS_NONE, 
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}

		// Creates a buffer to store modifiable acceleration datastructure geometry and instance buffers.
		// This resource is treated efficiently on the gpu and can be written at the beggining
		// using the uploading version.
		template<typename T>
		gObj<Buffer> RWAccelerationDatastructureBuffer(int count) {
			return GenericBuffer<T>(D3D12_RESOURCE_STATE_COMMON, count, CPU_ACCESS_NONE,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}
	};

	// Represents the loading module of a device manager.
	// This class is used to load other techniques and pipeline objects.
	class Loading {
		friend DeviceManager;

		gObj<DeviceManager> manager;
		Loading(gObj<DeviceManager> manager) :manager(manager) {}
	public:
		// Loads a technique can be used as child technique.
		// Nevertheless, this engine proposes multiple inheritance of techniques instead for reusing logics.
		template<typename T>
		void Subprocess(gObj<T> &technique)
		{
			technique->__OnInitialization(this->manager);
		}

		// Loads a pipeline object of a specific type.
		// Receives the variable to save the pipeline
		// The object is created in openned state, so, must be closed before being set on the gpu for drawing.
		template<typename P, typename... A>
		void Pipeline(gObj<P> &pipeline, A... args) {
			pipeline = new P(args...);
			pipeline->__OnInitialization(this->manager);
			pipeline->Close();
		}
	};


}

#pragma endregion

#pragma region ca4GCommandLists.h

namespace CA4G {
#pragma region Managing Command Lists

	// Wrapper to a CommandList object of DX. This class is inteded for be inherited.
	class CommandListManager {
		friend Presenter;
		friend Creating;
		friend Copying;
		friend GPUScheduler;
		friend TriangleGeometryCollection;

		// Indicates the command list was closed and can be sent to the allocator
		bool closed;

		// Indicates this command list has something to send to the allocator
		bool active = false;

		DX_Device getInternalDevice();

		bool EquivalentDesc(const D3D12_RESOURCE_DESC &d1, const D3D12_RESOURCE_DESC &d2) {
			return d1.Width == d2.Width && d1.Height == d2.Height && d1.DepthOrArraySize == d2.DepthOrArraySize;
		}

		virtual void BooForVT() { }

		// Use this command list to copy from one resource to another
		void __All(gObj<ResourceWrapper> &dst, gObj<ResourceWrapper> &src);

	protected:

		list<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescriptors;
		list<D3D12_CPU_DESCRIPTOR_HANDLE> dstDescriptors;
		list<unsigned int> dstDescriptorRangeLengths;

		DX_CommandList cmdList;
		gObj<DeviceManager> manager;

		CommandListManager(gObj<DeviceManager> manager, DX_CommandList cmdList) :
			manager(manager),
			cmdList(cmdList),
			clearing(new Clearing(this, cmdList)),
			copying(new Copying(this, cmdList)),
			loading(new Loading(this))
		{
		}

		// Gets or sets the tag set by the device manager for the process is using this command list manager
		int Tag;

	public:

		// Gets the tag set by the device manager for the process is using this command list manager
		inline int getTag() {
			return Tag;
		}

		// Allows access to all clearing functions for Resources
		class Clearing {
			DX_CommandList cmdList;
			CommandListManager* manager;
		public:
			Clearing(CommandListManager* manager, DX_CommandList cmdList) :manager(manager), cmdList(cmdList) {}

			inline void UAV(gObj<ResourceView> uav, const FLOAT values[4]) {
				uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				long gpuHandleIndex = this->manager->manager->descriptors->gpu_csu->Malloc(1);
				auto cpuHandleAtVisibleHeap = this->manager->manager->descriptors->gpu_csu->getCPUVersion(gpuHandleIndex);
				auto gpuHandle = this->manager->manager->descriptors->gpu_csu->getGPUVersion(gpuHandleIndex);
				auto cpuHandle = uav->getUAVHandle();
				manager->getInternalDevice()->CopyDescriptorsSimple(1, cpuHandleAtVisibleHeap, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cmdList->ClearUnorderedAccessViewFloat(
					gpuHandle,
					cpuHandle,
					uav->resource->internalResource, values, 0, nullptr); 
			}
			inline void UAV(gObj<ResourceView> uav, const float4 &value) {
				float v[4]{ value.x, value.y, value.z, value.w };
				uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				long gpuHandleIndex = this->manager->manager->descriptors->gpu_csu->Malloc(1);
				auto cpuHandleAtVisibleHeap = this->manager->manager->descriptors->gpu_csu->getCPUVersion(gpuHandleIndex);
				auto gpuHandle = this->manager->manager->descriptors->gpu_csu->getGPUVersion(gpuHandleIndex);
				auto cpuHandle = uav->getUAVHandle();
				manager->getInternalDevice()->CopyDescriptorsSimple(1, cpuHandleAtVisibleHeap, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cmdList->ClearUnorderedAccessViewFloat(
					gpuHandle,
					cpuHandle,
					uav->resource->internalResource, v, 0, nullptr);
			}
			inline void UAV(gObj<ResourceView> uav, const unsigned int &value) {
				unsigned int v[4]{ value, value, value, value };
				uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				long gpuHandleIndex = this->manager->manager->descriptors->gpu_csu->Malloc(1);
				auto cpuHandleAtVisibleHeap = this->manager->manager->descriptors->gpu_csu->getCPUVersion(gpuHandleIndex);
				auto gpuHandle = this->manager->manager->descriptors->gpu_csu->getGPUVersion(gpuHandleIndex);
				auto cpuHandle = uav->getUAVHandle();
				manager->getInternalDevice()->CopyDescriptorsSimple(1, cpuHandleAtVisibleHeap, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cmdList->ClearUnorderedAccessViewUint(
					gpuHandle,
					cpuHandle,
					uav->resource->internalResource, v, 0, nullptr);
			}
			inline void UAV(gObj<ResourceView> uav, const uint4 &value) {
				unsigned int v[4]{ value.x, value.y, value.z, value.w };
				uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				long gpuHandleIndex = this->manager->manager->descriptors->gpu_csu->Malloc(1);
				auto cpuHandleAtVisibleHeap = this->manager->manager->descriptors->gpu_csu->getCPUVersion(gpuHandleIndex);
				auto gpuHandle = this->manager->manager->descriptors->gpu_csu->getGPUVersion(gpuHandleIndex);
				auto cpuHandle = uav->getUAVHandle();
				manager->getInternalDevice()->CopyDescriptorsSimple(1, cpuHandleAtVisibleHeap, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cmdList->ClearUnorderedAccessViewUint(
					gpuHandle,
					cpuHandle,
					uav->resource->internalResource, v, 0, nullptr);
			}
			inline void UAV(gObj<ResourceView> uav, const unsigned int values[4]) {
				uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				long gpuHandleIndex = this->manager->manager->descriptors->gpu_csu->Malloc(1);
				auto cpuHandleAtVisibleHeap = this->manager->manager->descriptors->gpu_csu->getCPUVersion(gpuHandleIndex);
				auto gpuHandle = this->manager->manager->descriptors->gpu_csu->getGPUVersion(gpuHandleIndex);
				auto cpuHandle = uav->getUAVHandle();
				manager->getInternalDevice()->CopyDescriptorsSimple(1, cpuHandleAtVisibleHeap, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cmdList->ClearUnorderedAccessViewUint(
					gpuHandle,
					cpuHandle,
					uav->resource->internalResource, values, 0, nullptr);
			}
			inline void RT(gObj<Texture2D> rt, const FLOAT values[4]) {
				rt->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList->ClearRenderTargetView(rt->getRTVHandle(), values, 0, nullptr);
			}
			inline void RT(gObj<Texture2D> rt, const float4 &value) {
				float v[4]{ value.x, value.y, value.z, value.w };
				rt->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList->ClearRenderTargetView(rt->getRTVHandle(), v, 0, nullptr);
			}
			// Clears the render target accessed by the Texture2D View with a specific float3 value
			inline void RT(gObj<Texture2D> rt, const float3 &value) {
				rt->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				float v[4]{ value.x, value.y, value.z, 1.0f };
				cmdList->ClearRenderTargetView(rt->getRTVHandle(), v, 0, nullptr);
			}
			inline void Depth(gObj<Texture2D> depthStencil, float depth = 1.0f) {
				cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
			}
			inline void DepthStencil(gObj<Texture2D> depthStencil, float depth, UINT8 stencil) {
				cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
			}
			inline void Stencil(gObj<Texture2D> depthStencil, UINT8 stencil) {
				cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_STENCIL, 0, stencil, 0, nullptr);
			}
		}
		*const clearing;

		// Allows access to all copying functions from and to Resources
		class Copying {
			DX_CommandList cmdList;
			CommandListManager* manager;

			// Data is given by a reference using a pointer
			template<typename T>
			void GenericCopy(gObj<ResourceView> dst, T* data, const D3D12_BOX *box = nullptr) {
				if (dst->arraySliceCount > 1 || dst->mipSliceCount > 1)
					throw CA4GException::FromError(CA4G_Errors_Invalid_Operation, "Can not update a region of a resource of multiple subresources");

				int subresource = dst->mipSliceStart + dst->resource->desc.DepthOrArraySize * dst->arraySliceStart;

				if (dst->resource->cpu_access == CPU_WRITE_GPU_READ)
					dst->resource->Upload(data, subresource, box);
				else
				{
					dst->resource->CreateForUploading();
					dst->resource->uploadingResourceCache->Upload(data, subresource, box);
					dst->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
					manager->__All(dst->resource, dst->resource->uploadingResourceCache);
					//dst->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_COMMON);
				}
			}

		public:
			Copying(CommandListManager* manager, DX_CommandList cmdList) : manager(manager), cmdList(cmdList) {}

			template<typename T>
			void PtrDataToSubresource(gObj<Texture2D> dst, int mip, int slice, T* data, const D3D12_BOX *box = nullptr) {
				gObj<Texture2D> subresource = dst->CreateSlice(slice, 1, mip, 1);
				GenericCopy(subresource, data, box);
			}

			template<typename T>
			void PtrData(gObj<Buffer> dst, T* data, const D3D12_BOX *box = nullptr) {
				GenericCopy(dst, data, box);
			}

			// Data is given by an initializer list
			template<typename T>
			void ListData(gObj<Buffer> dst, std::initializer_list<T> data) {
				GenericCopy(dst, data.begin());
			}

			// Data is given expanded in a value object
			template<typename T>
			void ValueData(gObj<Buffer> dst, const T &data) {
				GenericCopy(dst, &data);
			}

			template<typename V>
			void All(gObj<V> & dst, gObj<V> & src) {
				dst->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
				src->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
				manager->__All(dst->resource, src->resource);
			}
		}
		*const copying;

		class Loading {
			CommandListManager *manager;
		public:
			Loading(CommandListManager* manager) :manager(manager) {}
			// Creates a 2D texture from a texture data object with an image loaded.
			void FromData(gObj<Texture2D> &texture, gObj<TextureData> data);

			// Creates a 2D texture from a specific file
			// Supported formats, dds, any wic image and tga.
			void FromFile(gObj<Texture2D> &texture, const char* filePath) {
				FromData(texture, TextureData::LoadFromFile(filePath));
			}

		}*const loading;
	};

	// Allows to access to the DX12's Copy engine interface.
	class CopyingManager : public CommandListManager {
		friend Presenter;
		friend DeviceManager;
		friend ComputeManager;
		friend GPUScheduler;

		CopyingManager(gObj<DeviceManager> manager, DX_CommandList cmdList) :CommandListManager(manager, cmdList)
		{
		}
	};

	// Allows to access to the DX12's Compute engine interface.
	class ComputeManager : public CopyingManager {
		friend Presenter;
		friend DeviceManager;
		friend GraphicsManager;
		friend DXRManager;
		friend GraphicsPipelineBindings;
		template<typename ...PPS> friend class PipelineBindings;
		friend GPUScheduler;
		friend IRTProgram;
		friend RTPipelineManager;

		// Gets the current pipeline object set
		gObj<IPipelineBindings> currentPipeline = nullptr;
		gObj<RTPipelineManager> currentPipeline1 = nullptr;
		gObj<IRTProgram> activeRTProgram = nullptr;

		ComputeManager(gObj<DeviceManager> manager, DX_CommandList cmdList) : CopyingManager(manager, cmdList), setting(new Setter(this))
		{
		}
	public:
		class Setter {
			ComputeManager *manager;
		public:
			Setter(ComputeManager* manager) :manager(manager) {
			}
			// Sets a graphics pipeline
			Setter* Pipeline(IPipelineBindings * pipeline) {
				manager->currentPipeline = pipeline;
				manager->cmdList->SetPipelineState(pipeline->pso);
				manager->cmdList->SetComputeRootSignature(pipeline->rootSignature);
				pipeline->__OnSet(manager);
				return this;
			}

		} *const setting;

	};

	// Allows to access to the DX12's Graphics engine interface.
	class GraphicsManager : public ComputeManager {
		friend Presenter;
		friend DeviceManager;
		friend GPUScheduler;
		friend DXRManager;
		friend gObj<GraphicsManager>;

		GraphicsManager(gObj<DeviceManager> manager, DX_CommandList cmdList) :ComputeManager(manager, cmdList),
			setting(new Setter(this)),
			dispatcher(new Dispatcher(this)) {
		}
		~GraphicsManager() {
			delete copying;
			delete setting;
			delete dispatcher;
		}

	public:
		class Setter {
			GraphicsManager *manager;
		public:
			Setter(GraphicsManager* manager) : manager(manager) {
			}

			// Sets a graphics pipeline
			Setter* Pipeline(gObj<IPipelineBindings> pipeline) {
				manager->currentPipeline = pipeline;
				manager->cmdList->SetPipelineState(pipeline->pso);
				manager->cmdList->SetGraphicsRootSignature(pipeline->rootSignature);
				pipeline->__OnSet(manager);
				return this;
			}

			// Changes the viewport
			Setter* Viewport(float width, float height, float maxDepth = 1.0f, float x = 0, float y = 0, float minDepth = 0.0f) {
				D3D12_VIEWPORT viewport;
				viewport.Width = width;
				viewport.Height = height;
				viewport.MinDepth = minDepth;
				viewport.MaxDepth = maxDepth;
				viewport.TopLeftX = x;
				viewport.TopLeftY = y;
				manager->cmdList->RSSetViewports(1, &viewport);

				D3D12_RECT rect;
				rect.left = (int)x;
				rect.top = (int)y;
				rect.right = (int)x + (int)width;
				rect.bottom = (int)y + (int)height;
				manager->cmdList->RSSetScissorRects(1, &rect);
				return this;
			}

			// Sets a vertex buffer in a specific slot
			Setter* VertexBuffer(gObj<Buffer> buffer, int slot = 0) {
				buffer->ChangeStateTo(manager->cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				D3D12_VERTEX_BUFFER_VIEW view;
				buffer->CreateVBV(view);
				manager->cmdList->IASetVertexBuffers(slot, 1, &view);
				return this;
			}

			// Sets an index buffer
			Setter* IndexBuffer(gObj<Buffer> buffer) {
				buffer->ChangeStateTo(manager->cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				D3D12_INDEX_BUFFER_VIEW view;
				buffer->CreateIBV(view);
				manager->cmdList->IASetIndexBuffer(&view);
				return this;
			}

		} *const setting;

		class Dispatcher {
			GraphicsManager *manager;
		public:
			Dispatcher(GraphicsManager* manager) :manager(manager) {
			}

			// Draws a primitive using a specific number of vertices
			void Primitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start = 0) {
				// Grant this cmdlist is active before use it
				if (manager->currentPipeline.isNull())
					return;

				manager->currentPipeline->__OnDraw(manager);
				manager->cmdList->IASetPrimitiveTopology(topology);
				manager->cmdList->DrawInstanced(count, 1, start, 0);
			}

			// Draws a primitive using a specific number of vertices
			void IndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY topology, int count, int start = 0) {
				// Grant this cmdlist is active before use it
				if (manager->currentPipeline.isNull())
					return;

				manager->currentPipeline->__OnDraw(manager);
				manager->cmdList->IASetPrimitiveTopology(topology);
				manager->cmdList->DrawIndexedInstanced(count, 1, start, 0, 0);
			}

			// Draws triangles using a specific number of vertices
			void Triangles(int count, int start = 0) {
				Primitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, count, start);
			}

			void IndexedTriangles(int count, int start = 0) {
				IndexedPrimitive(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, count, start);
			}
		}*const dispatcher;
	};

	// Allows to acccess to the DX12's DXR functionallities or Fallback device if not supported
	class DXRManager : public GraphicsManager {
		friend GPUScheduler;
		friend RTPipelineManager;
		//friend Dispatcher;
		friend GeometryCollection;
		friend InstanceCollection;
		friend IRTProgram;

		DX_FallbackCommandList fallbackCmdList;

		DXRManager(gObj<DeviceManager> manager, DX_CommandList cmdList) : GraphicsManager(manager, cmdList),
			setting(new Setter(this)),
			dispatcher(new Dispatcher(this)),
			creating(new Creating(this))
		{
			if (manager->fallbackDevice != nullptr)
				manager->fallbackDevice->QueryRaytracingCommandList(cmdList, IID_PPV_ARGS(&fallbackCmdList));
			else
				fallbackCmdList = nullptr;
		}
	public:
		class Setter {
			DXRManager* manager;
		public:
			Setter(DXRManager* manager) :manager(manager) {
			}
			void Pipeline(gObj<RTPipelineManager> bindings) {
				manager->currentPipeline1 = bindings;
				if (manager->fallbackCmdList != nullptr)
					manager->fallbackCmdList->SetPipelineState1(bindings->fbso);
				else
					manager->cmdList->SetPipelineState1(bindings->so);
				manager->currentPipeline1->__OnSet(this->manager);
			}

			void Program(gObj<IRTProgram> program) {
				manager->activeRTProgram = program;
				manager->cmdList->SetComputeRootSignature(program->globals->rootSignature);
				program->BindOnGPU(manager, program->globals);
			}

			// Commit all local bindings for this ray generation shader
			void RayGeneration(gObj<RayGenerationHandle> program) {
				manager->activeRTProgram->UpdateRayGenLocals(manager, program);
			}

			void Miss(gObj<MissHandle> program, int index) {
				manager->activeRTProgram->UpdateMissLocals(manager, program, index);
			}

			void HitGroup(gObj<HitGroupHandle> group, int geometryIndex,
				int rayContribution = 0, int multiplier = 1, int instanceContribution = 0)
			{
				int index = rayContribution + (geometryIndex * multiplier) + instanceContribution;
				manager->activeRTProgram->UpdateHitGroupLocals(manager, group, index);

			}

		} *const setting;

		class Dispatcher {
			DXRManager* manager;
		public:
			Dispatcher(DXRManager* manager) :manager(manager) {}
			void Rays(int width, int height, int depth = 1) {
				auto currentBindings = manager->currentPipeline1;
				auto currentProgram = manager->activeRTProgram;

				if (!currentBindings)
					return; // Exception?
				D3D12_DISPATCH_RAYS_DESC d;
				auto rtRayGenShaderTable = currentProgram->raygen_shaderTable;
				auto rtMissShaderTable = currentProgram->miss_shaderTable;
				auto rtHGShaderTable = currentProgram->group_shaderTable;

				auto DispatchRays = [&](auto commandList, auto stateObject, auto* dispatchDesc)
				{
					dispatchDesc->HitGroupTable.StartAddress = rtHGShaderTable->resource->GetGPUVirtualAddress();
					dispatchDesc->HitGroupTable.SizeInBytes = rtHGShaderTable->resource->desc.Width;
					dispatchDesc->HitGroupTable.StrideInBytes = rtHGShaderTable->Stride;

					dispatchDesc->MissShaderTable.StartAddress = rtMissShaderTable->resource->GetGPUVirtualAddress();
					dispatchDesc->MissShaderTable.SizeInBytes = rtMissShaderTable->resource->desc.Width;
					dispatchDesc->MissShaderTable.StrideInBytes = rtMissShaderTable->Stride;

					dispatchDesc->RayGenerationShaderRecord.StartAddress = rtRayGenShaderTable->resource->GetGPUVirtualAddress();
					dispatchDesc->RayGenerationShaderRecord.SizeInBytes = rtRayGenShaderTable->resource->desc.Width;

					dispatchDesc->Width = width;
					dispatchDesc->Height = height;
					dispatchDesc->Depth = depth;
					commandList->DispatchRays(dispatchDesc);
				};

				D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
				if (manager->fallbackCmdList)
				{
					DispatchRays(manager->fallbackCmdList, currentBindings->fbso, &dispatchDesc);
				}
				else // DirectX Raytracing
				{
					DispatchRays(manager->cmdList, currentBindings->so, &dispatchDesc);
				}
			}
		} *const dispatcher;

		class Creating {
			friend DXRManager;
			DXRManager* manager;
			Creating(DXRManager* manager) :manager(manager) {}
		public:
			// Creates an empty geometry collection to add triangle-based geometries.
			gObj<TriangleGeometryCollection> TriangleGeometries() {
				return new TriangleGeometryCollection(manager->manager, manager);
			}
			// Creates an empty geometry collection to add procedural geometries.
			gObj<ProceduralGeometryCollection> ProceduralGeometries() {
				return new ProceduralGeometryCollection(manager->manager, manager);
			}
			// Creates a geometry collection to update triangle-based geometries.
			gObj<TriangleGeometryCollection> TriangleGeometries(gObj<GeometriesOnGPU> updating) {
				return new TriangleGeometryCollection(manager->manager, manager, updating);
			}
			// Creates a geometry collection to update procedural geometries.
			gObj<ProceduralGeometryCollection> ProceduralGeometries(gObj<GeometriesOnGPU> updating) {
				return new ProceduralGeometryCollection(manager->manager, manager, updating);
			}
			gObj<InstanceCollection> Instances() {
				return new InstanceCollection(manager->manager, manager);
			}
			gObj<InstanceCollection> Instances(gObj<SceneOnGPU> updatingScene) {
				return new InstanceCollection(manager->manager, manager, updatingScene);
			}
		} *const creating;
	};

#pragma endregion
}

#pragma endregion

#pragma region ca4GTechniques.h

namespace CA4G {
	// Represents a technique. This class is intended to be speciallized by users.
// implementing methods for Startup (initialization) and Frame (per-frame) logics.
// Use getManager() to access to the underlaying DX12 device.
	class Technique {
		friend Presenter;
		friend Loading;

		bool isInitialized = false;
		void __OnInitialization(gObj<DeviceManager> manager);

		void __OnFrame() {
			Frame();
		}

		gObj<DeviceManager> manager;
	protected:
		// Loading module to load other techniques and pipelines
		Loading *loading;

		// Creating module to create resources.
		Creating *creating;

		// Gets access to DX12 device manager.
		inline gObj<DeviceManager> getManager() {
			return manager;
		}

		// This method will be called only once when application starts
		virtual void Startup() {
		}

		// This method will be called every frame
		virtual void Frame() {
		}

		// Triggers the frame execution of a technique.
		// Use this method from a technique to execute subtechniques
		template<typename T>
		void ExecuteFrame(gObj<T> &technique) {
			((gObj<Technique>)technique)->Frame();
		}

	public:
		Technique() {}
		virtual ~Technique() {
		}
	};
}

#pragma endregion

#pragma region ca4GPresenting.h


static const UUID D3D12RaytracingPrototype2 = { /* 5d15d3b2-015a-4f39-8d47-299ac37190d3 */
	0x5d15d3b2,
	0x015a,
	0x4f39,
{ 0x8d, 0x47, 0x29, 0x9a, 0xc3, 0x71, 0x90, 0xd3 }
};

namespace CA4G {

	// Represents a presenter object. This is a factory class that allows to create a 
	// DeviceManager object, render targets for triplebuffering, Swapchain for presenting.
	// Allows loading a technique and presenting it.
	class Presenter {

		// Enable experimental features required for compute-based raytracing fallback.
		// This will set active D3D12 devices to DEVICE_REMOVED state.
		// Returns bool whether the call succeeded and the device supports the feature.
		inline bool EnableComputeRaytracingFallback(CComPtr<IDXGIAdapter1> adapter)
		{
			ID3D12Device* testDevice;
			UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };

			return SUCCEEDED(D3D12EnableExperimentalFeatures(ARRAYSIZE(experimentalFeatures), experimentalFeatures, nullptr, nullptr))
				&& SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)));
		}

		// Returns bool whether the device supports DirectX Raytracing tier.
		inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
		{
			ID3D12Device* testDevice;
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

			return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
				&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
				&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
		}

		// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
		// If no such adapter can be found, *ppAdapter will be set to nullptr.
		_Use_decl_annotations_
			void GetHardwareAdapter(CComPtr<IDXGIFactory4> pFactory, CComPtr<IDXGIAdapter1> & ppAdapter)
		{
			IDXGIAdapter1* adapter;
			ppAdapter = nullptr;

			for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see if the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}

			ppAdapter = adapter;
		}

		gObj<Texture2D>* renderTargetViews;
		gObj<DeviceManager> manager;
		CComPtr<IDXGISwapChain3> swapChain;

		unsigned int CurrentBuffer;

		void __PrepareToPresent(gObj<GraphicsManager> cmds) {
			renderTargetViews[CurrentBuffer]->ChangeStateTo(cmds->cmdList, D3D12_RESOURCE_STATE_PRESENT);
		}

		void __PrepareToRenderTarget(gObj<GraphicsManager> cmds) {
			renderTargetViews[CurrentBuffer]->ChangeStateTo(cmds->cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

	protected:

		virtual void OnInitializeGUI() {

		}

		virtual void OnPresentGUI(D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle,
			DX_CommandList commandList) {

		}

	public:

		ID3D12Debug *debugController;

		DX_Device getInnerD3D12Device() {
			return manager->device;
		}

		DX_DescriptorHeap getDescriptorsHeapForGUI() {
			return manager->descriptors->gui_csu->heap;
		}

		// Creates a presenter object that creates de DX device attached to a specific window (hWnd).
		Presenter(HWND hWnd, bool fullScreen = false, int buffers = 2, bool useFrameBuffering = false, bool useWarpDevice = false) {
			UINT dxgiFactoryFlags = 0;
			DX_Device device;

#if defined(_DEBUG)
			// Enable the debug layer (requires the Graphics Tools "optional feature").
			// NOTE: Enabling the debug layer after device creation will invalidate the active device.
			{
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();

					// Enable additional debug layers.
					dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

					//((ID3D12Debug1*)debugController)->SetEnableGPUBasedValidation(true);
				}
			}
#endif

			CComPtr<IDXGIFactory4> factory;
#if _DEBUG
			CComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
			{
				CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
			else
				CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
#else
			CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
#endif

			if (useWarpDevice)
			{
				CComPtr<IDXGIAdapter> warpAdapter;
				factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

				//EnableDirectXRaytracing(warpAdapter);

				D3D12CreateDevice(
					warpAdapter,
					D3D_FEATURE_LEVEL_12_1,
					IID_PPV_ARGS(&device)
				);
			}
			else
			{
				CComPtr<IDXGIAdapter1> hardwareAdapter;
				GetHardwareAdapter(factory, hardwareAdapter);

				EnableComputeRaytracingFallback(hardwareAdapter);

				D3D12CreateDevice(
					hardwareAdapter,
					D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(&device)
				);
			}

			D3D12_FEATURE_DATA_D3D12_OPTIONS ops;
			device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &ops, sizeof(ops));

			manager = new DeviceManager(device, buffers, useFrameBuffering, useWarpDevice);

			RECT rect;
			GetClientRect(hWnd, &rect);

			// Describe and create the swap chain.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = buffers;
			swapChainDesc.Width = rect.right - rect.left;
			swapChainDesc.Height = rect.bottom - rect.top;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			IDXGISwapChain1 *tmpSwapChain;

			factory->CreateSwapChainForHwnd(
				manager->Scheduler->queues[0]->dxQueue,        // Swap chain needs the queue so that it can force a flush on it.
				hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&tmpSwapChain
			);

			swapChain = (IDXGISwapChain3*)tmpSwapChain;
			swapChain->SetMaximumFrameLatency(buffers);

			// This sample does not support fullscreen transitions.
			factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

			// Create rendertargets resources.
			{
				renderTargetViews = new gObj<Texture2D>[buffers];
				// Create a RTV and a command allocator for each frame.
				for (UINT n = 0; n < buffers; n++)
				{
					DX_Resource rtResource;
					swapChain->GetBuffer(n, IID_PPV_ARGS(&rtResource));

					gObj<ResourceWrapper> rtResourceWrapper = new ResourceWrapper(manager, rtResource->GetDesc(), rtResource, D3D12_RESOURCE_STATE_COPY_DEST);
					renderTargetViews[n] = new Texture2D(rtResourceWrapper);
				}
			}

			CurrentBuffer = swapChain->GetCurrentBackBufferIndex();
			manager->BackBuffer = renderTargetViews[CurrentBuffer];
			/*
					manager->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gui_allocator[0]));
					manager->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gui_allocator[1]));
					manager->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gui_allocator[2]));
					manager->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gui_allocator[0], nullptr, IID_PPV_ARGS(&gui_cmd));*/
		}

		// Loads a technique without initialize.
		template<typename T, typename... A>
		void Load(gObj<T> &technique, A... args)
		{
			technique = new T(args...);
		}

		// Presents a technique.
		void Present(gObj<Technique> technique) {

			CurrentBuffer = swapChain->GetCurrentBackBufferIndex();

			manager->Scheduler->SetupFrame(CurrentBuffer);

			static int frameIndex = 0;

			//manager->descriptors->gpu_csu->Reset((frameIndex++) % 3);
			manager->descriptors->gpu_csu->Reset(CurrentBuffer);

			manager->BackBuffer = renderTargetViews[CurrentBuffer];

			if (!technique->isInitialized)
			{
				technique->__OnInitialization(this->manager);
				manager->WaitFor(manager->SendSignal(manager->Flush(ENGINE_MASK_ALL)));
			}

			manager->Perform(Process(this, &Presenter::__PrepareToRenderTarget));

			//manager->Perform(process(__PrepareToRenderTarget));

			technique->__OnFrame();

#pragma region GUI

			// Before GUI, just in case render target 
			// finished in a wrong state
			manager->Perform(process(__PrepareToRenderTarget));

			manager->Flush(ENGINE_MASK_ALL);

			// Use temporarly CmdList of manager[0] for GUI drawing

			manager->Scheduler->engines[0].threadInfos[0].Activate(manager->Scheduler->engines[0].frames[CurrentBuffer].allocatorSet[0]);

			auto gui_cmd = manager->Scheduler->engines[0].threadInfos[0].cmdList;

			D3D12_CPU_DESCRIPTOR_HANDLE rthandle = manager->BackBuffer->getRTVHandle();

			OnPresentGUI(rthandle, gui_cmd);

			/*gui_cmd->OMSetRenderTargets(1, &rthandle, false, nullptr);
			gui_cmd->SetDescriptorHeaps(1, &this->manager->descriptors->gui_csu->heap);

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gui_cmd);
*/
#pragma endregion

			manager->Perform(process(__PrepareToPresent));

			manager->Flush(ENGINE_MASK_ALL);

			auto hr = swapChain->Present(0, 0);

			manager->Scheduler->FinishFrame();

			if (hr != S_OK) {
				HRESULT r = manager->device->GetDeviceRemovedReason();
				throw CA4GException::FromError(CA4G_Errors_Any, nullptr, r);
			}
		}

		// Flush all pending work, and release objects.
		void Close() {
			manager->WaitFor(manager->SendSignal(manager->Flush(ENGINE_MASK_ALL)));
		}
	};

}

#pragma endregion

#pragma region Model Loaders

namespace CA4G {

	// Default vertex definition for Scene vertex
	struct SCENE_VERTEX {
		float3 Position;
		float3 Normal;
		float2 Coordinates;
		float3 Tangent;
		float3 Binormal;

		static std::initializer_list<VertexElement> Layout() {
			static std::initializer_list<VertexElement> result{
				VertexElement(VertexElementType_Float, 3, "POSITION"),
				VertexElement(VertexElementType_Float, 3, "NORMAL"),
				VertexElement(VertexElementType_Float, 2, "TEXCOORD"),
				VertexElement(VertexElementType_Float, 3, "TANGENT"),
				VertexElement(VertexElementType_Float, 3, "BINORMAL")
			};

			return result;
		}
	};

	struct SCENE_MATERIAL {
		float3 Diffuse;
		float RefractionIndex;
		float3 Specular;
		float SpecularSharpness;
		int Diffuse_Map;
		int Specular_Map;
		int Bump_Map;
		int Mask_Map;
		float3 Emissive;
		float4 Roulette; // x-diffuse, y-specular, z-mirror, w-fresnel
		SCENE_MATERIAL() {
			Diffuse = float3(1, 1, 1);
			RefractionIndex = 1;
			Specular = float3(0, 0, 0);
			SpecularSharpness = 1;
			Diffuse_Map = -1;
			Specular_Map = -1;
			Bump_Map = -1;
			Mask_Map = -1;
			Emissive = float3(0, 0, 0);
			Roulette = float4(1, 0, 0, 0);
		}
	};

	struct SCENE_OBJECT {
		int startVertex;
		int vertexesCount;
		SCENE_MATERIAL* Material;
		float4x4* Transform;
	};

}

using namespace std;

namespace CA4G {

	class Tokenizer
	{
		char* buffer;
		int count;
		int pos = 0;
	public:
		Tokenizer(FILE* stream) {
			fseek(stream, 0, SEEK_END);
			fpos_t count;
			fgetpos(stream, &count);
			fseek(stream, 0, SEEK_SET);
			buffer = new char[count];
			int offset = 0;
			int read;
			do
			{
				read = fread_s(&buffer[offset], count, 1, min(count, 1024 * 1024 * 20), stream);
				count -= read;
				offset += read;
			} while (read > 0);
			this->count = offset;
		}
		~Tokenizer() {
			delete[] buffer;
		}
		inline bool isEof() {
			return pos == count;
		}

		inline bool isEol()
		{
			return isEof() || peek() == 10;
		}

		void skipCurrentLine() {
			while (!isEol()) pos++;
			if (!isEof()) pos++;
		}

		inline char peek() {
			return buffer[pos];
		}

		bool match(const char* token)
		{
			while (!isEof() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
				pos++;
			int initialPos = pos;
			int p = 0;
			while (!isEof() && token[p] == buffer[pos]) {
				p++; pos++;
			}
			if (token[p] == '\0')
				return true;
			pos = initialPos;
			return false;
		}
		bool matchDigit(int &d) {
			char ch = peek();

			if (ch >= '0' && ch <= '9')
			{
				d = ch - '0';
				pos++;
				return true;
			}
			return false;
		}
		bool matchSymbol(char c)
		{
			if (!isEof() && buffer[pos] == c)
			{
				pos++;
				return true;
			}
			return false;
		}

		string readTextToken() {

			int start = pos;
			while (!isEol() && buffer[pos] != ' ' && buffer[pos] != '/' && buffer[pos] != ';'&& buffer[pos] != ':'&& buffer[pos] != '.'&& buffer[pos] != ','&& buffer[pos] != '('&& buffer[pos] != ')')
				pos++;
			int end = pos - 1;
			return string((char*)(buffer + start), end - start + 1);
		}

		string readToEndOfLine()
		{
			int start = pos;
			while (!isEol())
				pos++;
			int end = pos - 1;
			if (!isEof()) pos++;
			return string((char*)(buffer + start), end - start + 1);
		}

		inline bool endsInteger(char c)
		{
			return (c < '0') || (c > '9');
		}

		void ignoreWhiteSpaces() {
			while (!isEol() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
				pos++;
		}

		bool readIntegerToken(int &i) {
			i = 0;
			if (isEol())
				return false;
			int initialPos = pos;
			ignoreWhiteSpaces();
			int sign = 1;
			if (buffer[pos] == '-')
			{
				sign = -1;
				pos++;
			}
			ignoreWhiteSpaces();
			int end = pos;
			while (pos < count && !endsInteger(buffer[pos])) {
				i = i * 10 + (buffer[pos] - '0');
				pos++;
			}
			i *= sign;

			if (pos > end)
				return true;
			pos = initialPos;
			return false;
		}
		bool readFloatToken(float &f) {
			int initialPos = pos;
			int sign = 1;
			ignoreWhiteSpaces();
			if (buffer[pos] == '-')
			{
				sign = -1;
				pos++;
			}
			int intPart = 0;
			float scale = 1;
			if ((pos < count && buffer[pos] == '.') || readIntegerToken(intPart))
			{
				f = intPart;
				if (pos < count && buffer[pos] == '.')
				{
					int fracPos = pos;
					int fracPart;
					pos++;
					if (!readIntegerToken(fracPart))
					{
						pos = initialPos;
						return false;
					}
					else
					{
						int expPart = 0;
						if (buffer[pos] == 'e') {
							pos++;
							readIntegerToken(expPart);
							scale = pow(10, expPart);
						}
					}
					float fracPartf = fracPart / pow(10, pos - fracPos - 1);
					f += fracPartf;
				}
				f *= sign * scale;
				return true;
			}
			pos = initialPos;
			return false;
		}
	};
}

namespace CA4G {
	class BaseLoader {

	public:
		SCENE_VERTEX* VerticesData = nullptr;
		int* ObjectsId = nullptr;
		int VerticesDataLength;

		int* MaterialIndices = nullptr;
		float4x4* TransformsData = nullptr;
		int ObjectsLength;
		SCENE_OBJECT* Objects = nullptr;

		list<gObj<TextureData>> Textures = { };
		list<string> textureNames = { };
		list<SCENE_MATERIAL> MaterialsData = { };
		list<string> materialNames = { };

		float3 Minim;
		float3 Maxim;

		virtual void Load(const char *fileName) = 0;

		void ComputeTangents()
		{
			for (int i = 0; i < VerticesDataLength; i += 3)
			{
				//Getting vertex from indices
				SCENE_VERTEX &vertex1 = VerticesData[i];
				SCENE_VERTEX &vertex2 = VerticesData[i + 1];
				SCENE_VERTEX &vertex3 = VerticesData[i + 2];

				//making 
				float3 edge_1_2 = vertex2.Position - vertex1.Position;
				float3 edge_1_3 = vertex3.Position - vertex1.Position;

				float2 coordinate_2_1 = vertex2.Coordinates - vertex1.Coordinates;
				float2 coordinate_3_1 = vertex3.Coordinates - vertex1.Coordinates;

				float alpha = 1.0f / (coordinate_2_1.x * coordinate_3_1.y - coordinate_2_1.y * coordinate_3_1.x);

				vertex1.Tangent = vertex2.Tangent = vertex3.Tangent = float3(
					((edge_1_2.x * coordinate_3_1.y) - (edge_1_3.x * coordinate_2_1.y)) * alpha,
					((edge_1_2.y * coordinate_3_1.y) - (edge_1_3.y * coordinate_2_1.y)) * alpha,
					((edge_1_2.z * coordinate_3_1.y) - (edge_1_3.z * coordinate_2_1.y)) * alpha
				);

				vertex1.Binormal = vertex2.Binormal = vertex3.Binormal = float3(
					((edge_1_2.x * coordinate_3_1.x) - (edge_1_3.x * coordinate_2_1.x)) * alpha,
					((edge_1_2.y * coordinate_3_1.x) - (edge_1_3.y * coordinate_2_1.x)) * alpha,
					((edge_1_2.z * coordinate_3_1.x) - (edge_1_3.z * coordinate_2_1.x)) * alpha
				);
			}

		}

	protected:

		void addMaterial(string name, SCENE_MATERIAL material) {
			materialNames.add(name);
			MaterialsData.add(material);
		}

		int resolveTexture(string subdir, string fileName) {
			for (int i = 0; i < textureNames.size(); i++)
				if (textureNames[i] == fileName)
					return i;

			string full = subdir;
			full += fileName;

			gObj<TextureData> data = TextureData::LoadFromFile(full.c_str());

			if (data.isNull()) // Image couldn't be loaded
			{
				return -1;
			}
			textureNames.add(fileName);
			Textures.add(data);
			return Textures.size() - 1;
		}

		int getMaterialIndex(string materialName) {
			for (int i = 0; i < materialNames.size(); i++)
				if (materialNames[i] == materialName)
					return i;
			materialName = "";
			return 0;
		}
	};
}

namespace CA4G {

	class ObjLoader : public BaseLoader {
	public:

		void importMtlFile(string subdir, string fileName)
		{
			string file = subdir;
			file += fileName;

			FILE* f;
			if (fopen_s(&f, file.c_str(), "r"))
				return;
			Tokenizer t(f);
			while (!t.isEof())
			{
				if (t.match("newmtl "))
				{
					addMaterial(t.readToEndOfLine(), SCENE_MATERIAL());
					continue;
				}

				if (t.match("Kd "))
				{
					float r, g, b;
					t.readFloatToken(r);
					t.readFloatToken(g);
					t.readFloatToken(b);
					float norm = max(0.0001f, max(r, max(g, b)));
					MaterialsData.last().Roulette.x = norm;
					MaterialsData.last().Diffuse = float3(r/norm, g/norm, b/norm);
					float rouletteXY = MaterialsData.last().Roulette.x + MaterialsData.last().Roulette.y;
					MaterialsData.last().Roulette.x /= rouletteXY;
					MaterialsData.last().Roulette.y /= rouletteXY;
					t.skipCurrentLine();
					continue;
				}

				if (t.match("Ks "))
				{
					float r, g, b;
					t.readFloatToken(r);
					t.readFloatToken(g);
					t.readFloatToken(b);
					float norm = max(0.0001f, max(r, max(g, b)));
					MaterialsData.last().Roulette.y = norm;
					MaterialsData.last().Specular = float3(r / norm, g / norm, b / norm);
					float rouletteXY = MaterialsData.last().Roulette.x + MaterialsData.last().Roulette.y;
					MaterialsData.last().Roulette.x /= rouletteXY;
					MaterialsData.last().Roulette.y /= rouletteXY;
					t.skipCurrentLine();
					continue;
				}

				if (t.match("Tr "))
				{
					float r, g, b;
					t.readFloatToken(r);
					if (r > 0) {
						//t.readFloatToken(g);
						//t.readFloatToken(b);
						MaterialsData.last().Roulette.x = (1 - r);
						MaterialsData.last().Roulette.y = (1 - r);
						MaterialsData.last().Roulette.z = r;
					}
					t.skipCurrentLine();
					continue;
				}

				if (t.match("Tf "))
				{
					float r, g, b;
					t.readFloatToken(r);
					t.readFloatToken(g);
					t.readFloatToken(b);
					float fresnel = (r + g + b) / 3;
					if (fresnel > 0)
					{
						MaterialsData.last().Roulette.x = (1 - fresnel);
						MaterialsData.last().Roulette.y = (1 - fresnel);
						MaterialsData.last().Roulette.w = fresnel;
					}
					t.skipCurrentLine();
					continue;
				}

				if (t.match("Ni "))
				{
					float ni;
					t.readFloatToken(ni);
					MaterialsData.last().RefractionIndex = ni;
					t.skipCurrentLine();
					continue;
				}

				if (t.match("Ns "))
				{
					float power;
					t.readFloatToken(power);
					MaterialsData.last().SpecularSharpness = power;
					t.skipCurrentLine();
					continue;
				}

				if (t.match("map_Kd "))
				{
					string textureName = t.readToEndOfLine();
					MaterialsData.last().Diffuse_Map = resolveTexture(subdir, textureName);
					continue;
				}

				if (t.match("map_Ks "))
				{
					string textureName = t.readToEndOfLine();
					MaterialsData.last().Specular_Map = resolveTexture(subdir, textureName);
					continue;
				}

				if (t.match("map_bump "))
				{
					string textureName = t.readToEndOfLine();
					MaterialsData.last().Bump_Map = resolveTexture(subdir, textureName);
					continue;
				}

				if (t.match("map_d "))
				{
					string textureName = t.readToEndOfLine();
					MaterialsData.last().Mask_Map = resolveTexture(subdir, textureName);
					continue;
				}

				t.skipCurrentLine();
			}
			fclose(f);
		}

		void addLineIndex(list<int> &indices, int index, int pos, int total) {
			if (index <= 0)
				index = total + index;

			if (pos <= 1)
				indices.add(index);
			else
			{
				indices.add(indices[indices.size() - 1]);
				indices.add(index);
			}
		}

		void ReadLineIndices(Tokenizer &t, list<int> &posIndices, list<int> &texIndices, list<int> &norIndices, int vCount, int vnCount, int vtCount)
		{
			int indexRead = 0;
			int pos = 0;
			int type = 0;

			bool p = false;
			bool n = false;
			bool te = false;
			while (!t.isEol())
			{
				int indexRead;
				if (t.readIntegerToken(indexRead))
				{
					switch (type)
					{
					case 0: addLineIndex(posIndices, (vCount + indexRead + 1) % (vCount + 1), pos, vCount);
						p = true;
						break;
					case 1: addLineIndex(texIndices, (vtCount + indexRead + 1) % (vtCount + 1), pos, vtCount);
						te = true;
						break;
					case 2: addLineIndex(norIndices, (vnCount + indexRead + 1) % (vnCount + 1), pos, vnCount);
						n = true;
						break;
					}
				}
				else
				{
					int a = 4;
					a = a + 5;
				}
				if (t.isEol())
				{
					if (p) {
						if (!n)
							addLineIndex(norIndices, 0, pos, vnCount);
						if (!te)
							addLineIndex(texIndices, 0, pos, vtCount);
						n = false;
						te = false;
						p = false;
					}
					t.skipCurrentLine();
					return;
				}
				if (t.matchSymbol('/'))
				{
					type++;
				}
				else
					if (t.matchSymbol(' '))
					{
						if (p) {
							if (!n)
								addLineIndex(norIndices, 0, pos, vnCount);
							if (!te)
								addLineIndex(texIndices, 0, pos, vtCount);
							n = false;
							te = false;
							p = false;
						}
						pos++;
						type = 0;
					}
					else
					{
						if (p) {
							if (!n)
								addLineIndex(norIndices, 0, pos, vnCount);
							if (!te)
								addLineIndex(texIndices, 0, pos, vtCount);
							n = false;
							te = false;
							p = false;
						}
						t.skipCurrentLine();
						return;
					}
			}
			if (p) {
				if (!n)
					addLineIndex(norIndices, 0, pos, vnCount);
				if (!te)
					addLineIndex(texIndices, 0, pos, vtCount);
			}
		}

		void addIndex(list<int> &indices, int index, int pos) {
			if (pos <= 2)
				indices.add(index);
			else
			{
				indices.add(indices[indices.size() - pos]); // vertex 0
				indices.add(indices[indices.size() - 1 - 1]);
				indices.add(index);
			}
		}

		void ReadFaceIndices(Tokenizer &t, list<int> &posIndices, list<int> &texIndices, list<int> &norIndices, int vCount, int vnCount, int vtCount)
		{
			int indexRead = 0;
			int pos = 0;
			int type = 0;

			bool p = false;
			bool n = false;
			bool te = false;
			while (!t.isEol())
			{
				int indexRead;
				if (t.readIntegerToken(indexRead))
				{
					switch (type)
					{
					case 0: addIndex(posIndices, (vCount + indexRead + 1) % (vCount + 1), pos);
						p = true;
						break;
					case 1: addIndex(texIndices, (vtCount + indexRead + 1) % (vtCount + 1), pos);
						te = true;
						break;
					case 2: addIndex(norIndices, (vnCount + indexRead + 1) % (vnCount + 1), pos);
						n = true;
						break;
					}
				}
				if (t.isEol())
				{
					if (p) {
						if (!n)
							addIndex(norIndices, 0, pos);
						if (!te)
							addIndex(texIndices, 0, pos);
						n = false;
						te = false;
						p = false;
					}
					t.skipCurrentLine();
					return;
				}
				if (t.matchSymbol('/'))
				{
					type++;
				}
				else
					if (t.matchSymbol(' '))
					{
						if (p) {
							if (!n)
								addIndex(norIndices, 0, pos);
							if (!te)
								addIndex(texIndices, 0, pos);
							n = false;
							te = false;
							p = false;
						}
						pos++;
						type = 0;
					}
					else
					{
						if (p) {
							if (!n)
								addIndex(norIndices, 0, pos);
							if (!te)
								addIndex(texIndices, 0, pos);
							n = false;
							te = false;
							p = false;
						}
						t.skipCurrentLine();
						return;
					}
			}
			if (p) {
				if (!n)
					addIndex(norIndices, 0, pos);
				if (!te)
					addIndex(texIndices, 0, pos);
			}
		}

		void Load(const char* filePath)
		{
			list<float3> positions;
			list<float3> normals;
			list<float2> texcoords;

			list<int> positionIndices;
			list<int> textureIndices;
			list<int> normalIndices;

			list<int> lpositionIndices;
			list<int> ltextureIndices;
			list<int> lnormalIndices;

			int offsetPos = 0;
			int offsetNor = 0;
			int offsetCoo = 0;

			string full(filePath);

			string subDir = full.substr(0, full.find_last_of('\\') + 1);
			string name = full.substr(full.find_last_of('\\') + 1);

			FILE* stream;

			Minim = float3(100000, 100000, 100000);
			Maxim = float3(-100000, -100000, -100000);

			errno_t err;
			if (err = fopen_s(&stream, filePath, "r"))
			{
				return;
			}

			list<int> groups;
			list<string> usedMaterials;

			Tokenizer t(stream);
			static int facecount = 0;

			while (!t.isEof())
			{
				if (t.match("v "))
				{
					float3 pos;
					t.readFloatToken(pos.x);
					t.readFloatToken(pos.y);
					t.readFloatToken(pos.z);

					Minim = minf(Minim, pos);
					Maxim = maxf(Maxim, pos);

					positions.add(pos);
					t.skipCurrentLine();
				}
				else
					if (t.match("vn ")) {
						float3 nor;
						t.readFloatToken(nor.x);
						t.readFloatToken(nor.y);
						t.readFloatToken(nor.z);
						normals.add(nor);
						t.skipCurrentLine();
					}
					else
						if (t.match("vt ")) {
							float2 coord;
							t.readFloatToken(coord.x);
							t.readFloatToken(coord.y);
							float z;
							t.readFloatToken(z);
							texcoords.add(coord);
							t.skipCurrentLine();
						}
						else
							if (t.match("l "))
							{
								ReadLineIndices(t, lpositionIndices, ltextureIndices, lnormalIndices, positions.size(), normals.size(), texcoords.size());
							}
							else
								if (t.match("f "))
								{
									ReadFaceIndices(t, positionIndices, textureIndices, normalIndices, positions.size(), normals.size(), texcoords.size());
								}
								else
									if (t.match("usemtl "))
									{
										string materialName = t.readToEndOfLine();
										groups.add(positionIndices.size());
										usedMaterials.add(materialName);
									}
									else
										if (t.match("mtllib ")) {
											string materialLibName = t.readToEndOfLine();
											importMtlFile(subDir, materialLibName);
										}
										else
											t.skipCurrentLine();
			}
			if (groups.size() == 0) // no material used
			{
				usedMaterials.add("default");
				groups.add(0);
			}
			groups.add(positionIndices.size());

			fclose(stream);

			float3* computedNormals = new float3[positions.size()];

			float3* lineVertices = new float3[lpositionIndices.size()];
			for (int i = 0; i < lpositionIndices.size(); i++)
				lineVertices[i] = positions[lpositionIndices[i] - 1];
			//LineVB = manager->builder->StructuredBuffer(lineVertices, lpositionIndices.getCount());

			VerticesDataLength = positionIndices.size();
			VerticesData = new SCENE_VERTEX[VerticesDataLength];
			ObjectsId = new int[VerticesDataLength];

			float scaleX = Maxim.x - Minim.x;
			float scaleY = Maxim.y - Minim.y;
			float scaleZ = Maxim.z - Minim.z;
			float scale = max(0.01f, max(scaleX, max(scaleY, scaleZ)));

			for (int i = 0; i < positionIndices.size(); i++)
			{
				VerticesData[i].Position = positions[positionIndices[i] - 1] / scale;
			}

			for (int i = 0; i < positionIndices.size(); i++)
				if (normalIndices[i] == 0)
				{
					float3 p0 = VerticesData[(i / 3) * 3 + 0].Position;
					float3 p1 = VerticesData[(i / 3) * 3 + 1].Position;
					float3 p2 = VerticesData[(i / 3) * 3 + 2].Position;
					computedNormals[positionIndices[i] - 1] = computedNormals[positionIndices[i] - 1] + (cross(p1 - p0, p2 - p0));
				}

			for (int i = 0; i < positionIndices.size(); i++)
				if (normalIndices[i] != 0)
					VerticesData[i].Normal = normals[normalIndices[i] - 1];
				else
				{
					VerticesData[i].Normal = normalize(computedNormals[positionIndices[i] - 1]);
					/*float3 p0 = VerticesData[(i / 3) * 3 + 0].Position;
					float3 p1 = VerticesData[(i / 3) * 3 + 1].Position;
					float3 p2 = VerticesData[(i / 3) * 3 + 2].Position;
					VerticesData[i].Normal = normalize(cross(p1 - p0, p2 - p0));*/
				}

			for (int i = 0; i < positionIndices.size(); i++)
				if (textureIndices[i] != 0)
					VerticesData[i].Coordinates = texcoords[textureIndices[i] - 1];
				else
				{
					float3 p = positions[positionIndices[i] - 1];
					VerticesData[i].Coordinates = p.getXY() / float2(scaleX, scaleY) - float2(0.5f + p.z*0.0001, 0.5f - p.z*0.0003);
				}

#pragma region Compute Tangents
			ComputeTangents();
#pragma endregion

			ObjectsLength = groups.size() - 1;

			MaterialIndices = new int[ObjectsLength];
			TransformsData = new float4x4[ObjectsLength];
			Objects = new SCENE_OBJECT[ObjectsLength];

			for (int i = 0; i < ObjectsLength; i++)
			{
				int startIndex = groups[i];
				int count = groups[i + 1] - groups[i];

				for (int j = 0; j < count; j++)
					ObjectsId[startIndex + j] = i;

				MaterialIndices[i] = getMaterialIndex(usedMaterials[i]);
				TransformsData[i] = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
				Objects[i] = {
					startIndex,
					count,
					&MaterialsData[MaterialIndices[i]],
					&TransformsData[i]
				};
			}

#ifdef SHUFFLE
#pragma region Shuffle
			for (int i = 1; i < positionIndices.getCount() / 3; i++)
			{
				int changeWith = rand() % i;
				for (int j = 0; j < 3; j++)
				{
					VERTEX tmp = vertices[i * 3 + j];
					vertices[i * 3 + j] = vertices[changeWith * 3 + j];
					vertices[changeWith * 3 + j] = tmp;

					int tmp2 = objectId[i * 3 + j];
					objectId[i * 3 + j] = objectId[changeWith * 3 + j];
					objectId[changeWith * 3 + j] = tmp2;
				}
			}
#pragma endregion
#endif
		}
	};
}

using namespace std;

namespace CA4G {

#pragma region Common scene definition objects

	// Represents a camera given a position and look target.
	class Camera
	{
	public:
		// Gets or sets the position of the observer
		float3 Position;
		// Gets or sets the target position of the observer
		// Can be used normalize(Target-Position) to refer to look direction
		float3 Target;
		// Gets or sets a vector used as constraint of camera normal.
		float3 Up;
		// Gets or sets the camera field of view in radians (vertical angle).
		float FoV;
		// Gets or sets the distance to the nearest visible plane for this camera.
		float NearPlane;
		// Gets or sets the distance to the farthest visible plane for this camera.
		float FarPlane;

		// Rotates this camera around Position
		void Rotate(float hor, float vert) {
			float3 dir = normalize(Target - Position);
			float3x3 rot = CA4G::Rotate(hor, Up);
			dir = mul(dir, rot);
			float3 R = cross(dir, Up);
			if (length(R) > 0)
			{
				rot = CA4G::Rotate(vert, R);
				dir = mul(dir, rot);
				Target = Position + dir;
			}
		}

		// Rotates this camera around Target
		void RotateAround(float hor, float vert) {
			float3 dir = (Position - Target);
			float3x3 rot = CA4G::Rotate(hor, float3(0, 1, 0));
			dir = mul(dir, rot);
			Position = Target + dir;
		}

		// Move this camera in front direction
		void MoveForward(float speed = 0.01) {
			float3 dir = normalize(Target - Position);
			Target = Target + dir * speed;
			Position = Position + dir * speed;
		}

		// Move this camera in backward direction
		void MoveBackward(float speed = 0.01) {
			float3 dir = normalize(Target - Position);
			Target = Target - dir * speed;
			Position = Position - dir * speed;
		}

		// Move this camera in right direction
		void MoveRight(float speed = 0.01) {
			float3 dir = Target - Position;
			float3 R = normalize(cross(dir, Up));

			Target = Target + R * speed;
			Position = Position + R * speed;
		}

		// Move this camera in left direction
		void MoveLeft(float speed = 0.01) {
			float3 dir = Target - Position;
			float3 R = normalize(cross(dir, Up));

			Target = Target - R * speed;
			Position = Position - R * speed;
		}

		// Gets the matrices for view and projection transforms
		void GetMatrices(int screenWidth, int screenHeight, float4x4 &view, float4x4 &projection)
		{
			view = LookAtRH(Position, Target, Up);
			projection = PerspectiveFovRH(FoV, screenHeight / (float)screenWidth, NearPlane, FarPlane);
		}
	};

	// Represents a light source definition
	// Point sources (Position: position of the light source, Direction = 0, SpotAngle = 0)
	// Directional sources (Position = 0, Direction: direction of the light source, SpotAngle = 0)
	// Spot lights (Position: position of the light source, Direction: direction of the spot light, SpotAngle: fov)
	class LightSource
	{
	public:
		float3 Position;
		float3 Direction;
		float SpotAngle;
		// Gets the intensity of the light
		float3 Intensity;
	};


#pragma endregion

	class Scene {

	public:

		inline SCENE_VERTEX* Vertices() {
			return loader->VerticesData;
		}
		inline int VerticesCount() {
			return loader->VerticesDataLength;
		}
		inline int* ObjectIds() {
			return loader->ObjectsId;
		}
		inline list<SCENE_MATERIAL>& Materials() {
			return loader->MaterialsData;
		}
		inline int MaterialsCount() {
			return loader->MaterialsData.size();
		}
		inline int ObjectsCount() {
			return loader->ObjectsLength;
		}
		inline int* MaterialIndices() {
			return loader->MaterialIndices;
		}
		inline SCENE_OBJECT* Objects() {
			return loader->Objects;
		}
		inline float4x4* Transforms() {
			return loader->TransformsData;
		}
		inline list<gObj<TextureData>>& Textures() {
			return loader->Textures;
		}

		Scene(const char* objFilePath) {

			int len = strlen(objFilePath);

			// It is obj model
			if (strcmp(objFilePath + len - 4, ".obj") == 0)
				loader = new ObjLoader();

			if (loader != nullptr)
				loader->Load(objFilePath);
		}
	private:
		BaseLoader* loader = nullptr;
	};
}

#pragma endregion

#pragma region DSL

#define render_target getManager()->getBackBuffer()

#define perform(method) getManager()->Perform(process(method))

#define perform_async(method) getManager()->PerformAsync(processPtr(method))

#define flush_to_gpu(engine_mask) getManager()->Flush(engine_mask)

#define flush_all_to_gpu getManager()->Flush(ENGINE_MASK_ALL)

#define signal(engine_mask) getManager()->SendSignal(engine_mask)

#define wait_for(signal) getManager()->WaitFor(signal)

#define set_tag(t) getManager()->Tag = t

#define _ this

#define gBind(x) (x) =

#define gCreate ->creating->

#define gLoad ->loading->

#define gSet ->setting->

#define gDispatch ->dispatcher->

#define gOpen(pipeline) (pipeline)->Open();

#define gClose(pipeline) (pipeline)->Close();

#define gClear ->clearing->

#define gCopy ->copying->

#pragma endregion

#pragma region Implementation for Generic definitions

namespace CA4G {
	template<typename ...A>
	void PipelineBindings<A...>::BindOnGPU(ComputeManager* manager, const list<SlotBinding> &list, int startRootParameter) {
		DX_CommandList cmdList = manager->cmdList;

		// Foreach bound slot
		for (int i = 0; i < list.size(); i++)
		{
			SlotBinding binding = list[i];

			// Gets the range length (if bound an array) or 1 if single.
			int count = binding.ptrToCount == nullptr ? 1 : *binding.ptrToCount;

			// Gets the bound resource if single
			gObj<ResourceView> resource = binding.ptrToCount == nullptr ? *((gObj<ResourceView>*)binding.ptrToResource) : nullptr;

			// Gets the bound resources if array or treat the resource as a single array case
			gObj<ResourceView>* resourceArray = binding.ptrToCount == nullptr ? &resource
				: *((gObj<ResourceView>**)binding.ptrToResource);

			// foreach resource in bound array (or single resource treated as array)
			for (int j = 0; j < count; j++)
			{
				// reference to the j-th resource (or bind null if array is null)
				gObj<ResourceView> resource = resourceArray == nullptr ? nullptr : *(resourceArray + j);

				if (!resource)
					// Grant a resource view to create null descriptor if missing resource.
					resource = ResourceView::getNullView(this->manager, binding.Dimension);
				else
				{
					switch (binding.type)
					{
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						resource->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_GENERIC_READ);
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						resource->ChangeStateToUAV(cmdList);
						break;
					}
				}
				// Gets the cpu handle at not visible descriptor heap for the resource
				D3D12_CPU_DESCRIPTOR_HANDLE handle;
				resource->getCPUHandleFor(binding.type, handle);

				// Adds the handle of the created descriptor into the src list.
				manager->srcDescriptors.add(handle);
			}
			// add the descriptors range length
			manager->dstDescriptorRangeLengths.add(count);
			int startIndex = this->manager->descriptors->gpu_csu->Malloc(count);
			manager->dstDescriptors.add(this->manager->descriptors->gpu_csu->getCPUVersion(startIndex));
			cmdList->SetGraphicsRootDescriptorTable(startRootParameter + i, this->manager->descriptors->gpu_csu->getGPUVersion(startIndex));
		}
	}

	template<typename ...A>
	void PipelineBindings<A...>::__OnSet(ComputeManager* manager) {

		DX_CommandList cmdList = manager->cmdList;
		for (int i = 0; i < RenderTargetMax; i++)
			if (!(*RenderTargets[i]))
				RenderTargetDescriptors[i] = ResourceView::getNullView(this->manager, D3D12_RESOURCE_DIMENSION_TEXTURE2D)->getRTVHandle();
			else
			{
				(*RenderTargets[i])->ChangeStateTo(manager->cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
				RenderTargetDescriptors[i] = (*RenderTargets[i])->getRTVHandle();
			}
		D3D12_CPU_DESCRIPTOR_HANDLE depthHandle;
		if (!(DepthBufferField == nullptr || !(*DepthBufferField)))
			depthHandle = (*DepthBufferField)->getDSVHandle();

		cmdList->OMSetRenderTargets(RenderTargetMax, RenderTargetDescriptors, FALSE, (DepthBufferField == nullptr || !(*DepthBufferField)) ? nullptr : &depthHandle);
		ID3D12DescriptorHeap* heaps[] = { this->manager->descriptors->gpu_csu->getInnerHeap(), this->manager->descriptors->gpu_smp->getInnerHeap() };
		cmdList->SetDescriptorHeaps(2, heaps);

		BindOnGPU(manager, __GlobalsCSU, 0);
	}
}

namespace CA4G {
	template<typename S>
	void gObj<S>::AddReference() {
		if (!counter)
			throw new CA4GException("Error referencing");
		
		InterlockedAdd(counter, 1);
	}

	template<typename S>
	void gObj<S>::RemoveReference() {
		if (!counter)
			throw new CA4GException("Error referencing");

		InterlockedAdd(counter, -1);
		if ((*counter) == 0) {
			delete _this;
			delete counter;
			//_this = nullptr;
			counter = nullptr;
		}
	}

	template<typename T>
	bool ProducerConsumerQueue<T>::TryConsume(T& element)
	{
		if (productsSemaphore.Wait())
		{
			// ensured there is an element to be consumed
			mutex.Acquire();
			element = elements[start];
			start = (start + 1) % elementsLength;
			count--;
			spacesSemaphore.Release();
			mutex.Release();
			return true;
		}
		return false;
	}

	template<typename T>
	bool ProducerConsumerQueue<T>::TryProduce(T element) {
		if (spacesSemaphore.Wait())
		{
			mutex.Acquire();
			int enqueuePos = (start + count) % elementsLength;
			elements[enqueuePos] = element;
			count++;
			productsSemaphore.Release();
			mutex.Release();
			return true;
		}
		return false;
	}
}


namespace CA4G {
	template<typename T>
	void ResourceWrapper::Upload(T* data, int subresource, const D3D12_BOX *box) {
		D3D12_RANGE range{ };
		if (mappedData == nullptr)
		{
			mutex.Acquire();
			if (mappedData == nullptr)
			{
				auto hr = internalResource->Map(0, &range, &mappedData);
				if (FAILED(hr))
					throw CA4GException::FromError(CA4G_Errors_Any, nullptr, hr);
			}
			mutex.Release();
		}

		D3D12_BOX fullBox{ 0, 0, 0, pLayouts[subresource].Footprint.Width, pLayouts[subresource].Footprint.Height, desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize };

		if (box == nullptr)
			box = &fullBox;

		UINT64 bytesPerElement = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? 1 : sizeof(T);
		UINT64 srcRowPitch = (box->right - box->left)*bytesPerElement;
		UINT64 srcSlicePitch = (box->bottom - box->top)*srcRowPitch;

		UINT64 bytesPerRow = pLayouts[subresource].Footprint.RowPitch;
		UINT64 bytesPerSlice = pLayouts[subresource].Footprint.Height * bytesPerRow;
		for (int slice = box->front; slice < box->back; slice++)
		{
			byte* dstStartSlicePtr = ((byte*)mappedData + pLayouts[subresource].Offset) + slice * bytesPerSlice;
			const byte* srcStartSlicePtr = ((byte*)data) + slice * srcSlicePitch;
			for (int row = box->top; row < box->bottom; row++)
				memcpy(
					dstStartSlicePtr + row * bytesPerRow + box->left*bytesPerElement,
					srcStartSlicePtr + row * srcRowPitch, srcRowPitch);
		}

		//internalResource->Unmap(subresource, nullptr); // TODO: optimize this.
	}

	template<typename R>
	DX_RootSignature RTProgram<R>::CreateRootSignature(gObj<BindingsHandle> bindings, int &size) {
		size = 0;

		list<SlotBinding> &csu = bindings->csuBindings;
		list<SlotBinding> &samplers = bindings->samplerBindings;

		D3D12_ROOT_PARAMETER * parameters = new D3D12_ROOT_PARAMETER[csu.size() + samplers.size()];
		D3D12_DESCRIPTOR_RANGE * ranges = new D3D12_DESCRIPTOR_RANGE[csu.size() + samplers.size()];
		int index = 0;
		for (int i = 0; i < csu.size(); i++)
		{
			parameters[index] = csu[i].Root_Parameter;

			switch (csu[i].Root_Parameter.ParameterType) {
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				size += csu[i].Root_Parameter.Constants.Num32BitValues * 4;
				break;
			default:
				size += 4 * 4;
				break;
			}

			index++;
		}
		for (int i = 0; i < samplers.size(); i++)
		{
			parameters[index] = samplers[i].Root_Parameter;
			size += 16;
			index++;
		}

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.pParameters = parameters;
		desc.NumParameters = index;
		desc.pStaticSamplers = bindings->Samplers;
		desc.NumStaticSamplers = bindings->Max_Sampler;
		desc.Flags = bindings->isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;

		CComPtr<ID3DBlob> signatureBlob;
		CComPtr<ID3DBlob> signatureErrorBlob;

		DX_RootSignature rootSignature;

		if (manager->fallbackDevice != nullptr) // will use fallback for DX RT support
		{
			auto hr = manager->fallbackDevice->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
			if (hr != S_OK)
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, ("Error serializing root signature"));
			hr = manager->fallbackDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
			if (hr != S_OK)
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, ("Error creating root signature"));
		}
		else
		{
			auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
			if (hr != S_OK)
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, ("Error serializing root signature"));

			hr = manager->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
			if (hr != S_OK)
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, ("Error creating root signature"));
		}

		delete[] parameters;

		return rootSignature;
	}

	template<typename R>
	void RTProgram<R>::__OnInitialization(gObj<DeviceManager> manager, RTPipelineManager* rtManager) {
		this->manager = manager;
		this->_rt_manager = dynamic_cast<R*>(rtManager);// .Dynamic_Cast<R>();
		Setup();
		// Build and append root signatures
		globals = new BindingsHandle();
		__CurrentBindings = globals;
		Globals();
		BuildRootSignatureForCurrentBindings();

		raygen_locals = new BindingsHandle();
		raygen_locals->isLocal = true;
		__CurrentBindings = raygen_locals;
		RayGeneration_Locals();
		if (raygen_locals->HasSomeBindings())
			BuildRootSignatureForCurrentBindings();
		else
			raygen_locals = nullptr;

		miss_locals = new BindingsHandle();
		miss_locals->isLocal = true;
		__CurrentBindings = miss_locals;
		Miss_Locals();
		if (miss_locals->HasSomeBindings())
			BuildRootSignatureForCurrentBindings();
		else
			miss_locals = nullptr;

		hitGroup_locals = new BindingsHandle();
		hitGroup_locals->isLocal = true;
		__CurrentBindings = hitGroup_locals;
		HitGroup_Locals();
		if (hitGroup_locals->HasSomeBindings())
			BuildRootSignatureForCurrentBindings();
		else
			hitGroup_locals = nullptr;

		// Get shader identifiers.
		UINT shaderIdentifierSize;
		if (manager->fallbackDevice != nullptr)
			shaderIdentifierSize = manager->fallbackDevice->GetShaderIdentifierSize();
		else // DirectX Raytracing
			shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		raygen_shaderTable = manager->creating->ShaderTable(
			D3D12_RESOURCE_STATE_GENERIC_READ, shaderIdentifierSize +
			(raygen_locals.isNull() ? 0 : raygen_locals->rootSize), 1);
		miss_shaderTable = manager->creating->ShaderTable(
			D3D12_RESOURCE_STATE_GENERIC_READ, shaderIdentifierSize +
			(miss_locals.isNull() ? 0 : miss_locals->rootSize), MaxMiss);
		group_shaderTable = manager->creating->ShaderTable(
			D3D12_RESOURCE_STATE_GENERIC_READ, shaderIdentifierSize +
			(hitGroup_locals.isNull() ? 0 : hitGroup_locals->rootSize), MaxGroups);
	}

	template<int count>
	D3D12_SHADER_BYTECODE ShaderLoader::FromMemory(const byte(&bytecodeData)[count]) {
		D3D12_SHADER_BYTECODE code;
		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecodeData;
		return code;
	}
}
#pragma endregion

#endif