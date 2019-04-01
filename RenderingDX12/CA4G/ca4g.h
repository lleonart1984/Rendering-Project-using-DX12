//-------------------------------------------------------------------------------------
// ca4g.h
//  
// Main classes facade of DX12
//
// Copyright (c) Technical University of Munich and University of Havana. All rights reserved.
// Licensed under the MIT License.
//
//-------------------------------------------------------------------------------------
#pragma once

#include <comdef.h>
#include "..\Imgui\imgui.h"
#include "..\Imgui\imgui_impl_dx12.h"
#include <string>
#include <stdio.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "D3D12RaytracingFallback.h"
#include <Windows.h>
#include "gmath.h"
#include "ca4GMemory.h"
#include "list.h"
#include "ca4gApplication.h"
#include "ca4gSync.h"
#include "ca4gFormats.h"
#include "ca4gImaging.h"
#include "ca4gDSL.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "Shaders\DrawScreen_VS.hlsl.h"
#include "Shaders\DrawScreen_PS.hlsl.h"
#include "Shaders\DrawComplexity_PS.hlsl.h"

#define CA4G_MAX_NUMBER_OF_WORKERS 8
#define CA4G_SUPPORTED_ENGINES 4
#define CA4G_SUPPORTED_BUFFERING 3

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
class GPUScheduler;
class SceneBuilder;
class TriangleGeometryCollection;
class GeometryCollection;
class InstanceCollection;

class HitGroupManager;
template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type> class DynamicStateBindingOf;
class DXILManager;
template<typename C> class DXIL_Library;
class IRTProgram;
template<typename L> class RTProgram;
class RTPipelineManager;
#pragma endregion

#pragma region TOOLS for MACROS

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

#define ENGINE_MASK_ALL ((1 << CA4G_SUPPORTED_ENGINES) - 1)

#define ENGINE_MASK_DIRECT 1
#define ENGINE_MASK_BUNDLE 2
#define ENGINE_MASK_COMPUTE 4
#define ENGINE_MASK_COPY 8

#define CompiledShader(s) (s),ARRAYSIZE(s) 

#pragma endregion

#pragma region Enums

// Represents the accessibility optimization of a resource by the cpu
typedef enum CPU_ACCESS {
	// The CPU can not read or write
	CPU_ACCESS_NONE = 0,
	// The CPU can write and GPU can access frequently efficiently
	CPU_WRITE_GPU_READ = 1,
	// The GPU can write once and CPU can access frequently efficiently
	CPU_READ_GPU_WRITE = 2
} CPU_ACCESS;

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

#pragma endregion

#pragma region Vertex Description

DXGI_FORMAT TYPE_VS_COMPONENTS_FORMATS[3][4]{
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

#pragma region Managing Descriptors

// Represents a CPU free descriptor heap manager
// Can be used to create descriptors on CPU and the allocate arranged on the GPU descriptor when resources are bound to the pipeline
class CPUDescriptorHeapManager {
	friend Presenter;
	ID3D12DescriptorHeap *heap;
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
	CPUDescriptorHeapManager(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity);
	// Allocates a new index for a resource descriptor
	int AllocateNewHandle();
	// Releases a specific index of a resource is being released.
	inline void Release(int index) { free = new Node{ index, free }; }
	inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }
	inline ID3D12DescriptorHeap* getInnerHeap() { return heap; }
};

// Represents a GPU descriptor heap manager
// Is used to allocate descriptors on GPU sequentially when resources are bound to the pipeline
class GPUDescriptorHeapManager {
	friend Presenter;
	ID3D12DescriptorHeap *heap;
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

	void Reset(int frameIndex) {
		mallocOffset = frameIndex * capacity / 3;
		blockCapacity = (frameIndex + 1)*capacity / 3;
	}

	/*long Malloc(int descriptors) {
		auto result = InterlockedExchangeAdd(&mallocOffset, descriptors);

		if (mallocOffset >= capacity)
			throw AfterShow(CA4G_Errors_RunOutOfMemory, TEXT("Desc Heap"));

		return result;
	}*/

	long Malloc(int descriptors) {
		auto result = InterlockedExchangeAdd(&mallocOffset, descriptors);
		if (mallocOffset >= blockCapacity)
			throw AfterShow(CA4G_Errors_RunOutOfMemory, TEXT("Descriptor heap"));
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

	GPUDescriptorHeapManager(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity);
	// Gets the cpu descriptor handle to access to a specific index slot of this descriptor heap
	inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUVersion(int index) { return D3D12_GPU_DESCRIPTOR_HANDLE{ startGPU + index * size }; }
	// Gets the gpu descriptor handle to access to a specific index slot of this descriptor heap
	inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUVersion(int index) { return D3D12_CPU_DESCRIPTOR_HANDLE{ startCPU + index * size }; }

	inline ID3D12DescriptorHeap* getInnerHeap() { return heap; }
};

// Represents a global manager of all static descriptor heaps needed.
// Static descriptors will be accessed by the application and any threaded command list manager
// Each trheaded command list manager will have their own gpu descriptors for csu and samplers.
class DescriptorsManager
{
public:
	// -- Shader visible heaps --
	
	// Descriptor heap for gui windows and fonts
	GPUDescriptorHeapManager* const gui_csu;
	GPUDescriptorHeapManager* const gpu_csu;
	GPUDescriptorHeapManager* const gpu_smp;
	
	// -- CPU heaps --
	
	// Descriptor heap for render targets views
	CPUDescriptorHeapManager* const cpu_rt;
	// Descriptor heap for depth stencil views
	CPUDescriptorHeapManager* const cpu_ds;
	// Descriptor heap for cbs, srvs and uavs in CPU memory
	CPUDescriptorHeapManager* const cpu_csu;
	// Descriptor heap for sampler objects in CPU memory
	CPUDescriptorHeapManager* const cpu_smp;

	DescriptorsManager(ID3D12Device *device) :
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

#pragma endregion

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
	virtual void Call(CommandListManager* manager) = 0;
private:
	int TagID = 0;
};



// Represents a callable member used as a process of a specific command list manager type (copy, compute or graphics)
template<typename T, typename A>
struct CallableMember : public ICallableMember {
	T* instance;
	typedef void(T::*Member)(A*);
	Member function;
	CallableMember(T* instance, Member function) : instance(instance), function(function) {
	}
	void Call(CommandListManager* manager) {
		(instance->*function)((A*)manager);
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

#define process(methodName) Process(this, &decltype(dr(this))::methodName)

#define processPtr(methodName, tagID) ProcessPtr(this, &decltype(dr(this))::methodName)

#pragma endregion

#pragma region GPU Scheduler

class GPUScheduler {
	friend DeviceManager;
	friend Presenter;

	// Struct for threading parameters
	struct GPUWorkerInfo {
		int Index;
		GPUScheduler* Scheduler;
	} * workers;

	// Represents a wrapper for command queue functionalities
	class CommandQueueManager {
		friend GPUScheduler;
		friend Presenter;

		ID3D12CommandQueue *dxQueue;
		long fenceValue;
		ID3D12Fence* fence;
		HANDLE fenceEvent;
		D3D12_COMMAND_LIST_TYPE type;
	public:
		CommandQueueManager(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type) : type(type) {
			D3D12_COMMAND_QUEUE_DESC d = {};
			d.Type = type;
			device->CreateCommandQueue(&d, IID_PPV_ARGS(&dxQueue));

			fenceValue = 1;
			device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

			fenceEvent = CreateEventW(nullptr, false, false, nullptr);
		}

		// Executes a command list set in this engine
		inline void Commit(ID3D12GraphicsCommandList** cmdLists, int count) {
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
			CommandListManager* manager;
			ID3D12GraphicsCommandList5* cmdList;
			bool isActive;
			// Prepares this command list for recording. This method resets the command list to use the desiredAllocator.
			void Activate(ID3D12CommandAllocator* desiredAllocator) {
				if (isActive)
					return;
				cmdList->Reset(desiredAllocator, nullptr);
				isActive = true;
			}

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
			ID3D12CommandAllocator** allocatorSet;
			bool* allocatorsUsed;

			inline ID3D12CommandAllocator* RequireAllocator(int index) {
				allocatorsUsed[index] = true;
				return allocatorSet[index];
			}

			void ResetUsedAllocators(int threads) {
				for (int i = 0; i < threads; i++)
				{
					allocatorsUsed[i] = false;
					allocatorSet[i]->Reset();
				}
			}
		} *frames;

	} engines[CA4G_SUPPORTED_ENGINES];

	// Used by flush method to call execute command list for each engine
	ID3D12GraphicsCommandList** __activeCmdLists;

	DeviceManager * manager;
	HANDLE* threads;
	ProducerConsumerQueue<ICallableMember*>* workToDo;
	CountEvent *counting;
	int threadsCount;
	int currentFrameIndex;
	// Gets whenever the user wants to synchronize frame rendering using frame buffering
	bool useFrameBuffer;

	Signal* perFrameFinishedSignal;

	static DWORD WINAPI __WORKER_TODO(LPVOID param);

	GPUScheduler(DeviceManager* manager, bool useFrameBuffer, int max_threads = CA4G_MAX_NUMBER_OF_WORKERS, int buffers = CA4G_SUPPORTED_BUFFERING);
	
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

	HANDLE __fencesForWaiting [CA4G_SUPPORTED_ENGINES];

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

#pragma region Device Manager

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

	GPUScheduler * Scheduler;
	gObj<Texture2D> BackBuffer;
	CountEvent *counting;
	DescriptorsManager * const descriptors;
	// Device object used to manage DX functionalities
	// Supported DXR interface here
	ID3D12Device5 * const device;

	// Fallback device object used to manage DXR functionalities when no GPU support.
	ID3D12RaytracingFallbackDevice *fallbackDevice;

	gObj<Buffer> __NullBuffer = nullptr;
	gObj<Texture2D> __NullTexture2D = nullptr;

	DeviceManager(ID3D12Device5 *device, int buffers, bool useFrameBuffer, bool isWarpDevice);

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
	inline void Perform(CallableMember<T, CopyingManager>& process) {
		process.TagID = Tag;
		Scheduler->Enqueue(&process);
	}
	template<typename T>
	// Performs a compute limited task synchronously.
	inline void Perform(CallableMember<T, ComputeManager>& process) {
		process.TagID = Tag;
		Scheduler->Enqueue(&process);
	}
	// Performs a general graphics task synchronously.
	template<typename T>
	inline void Perform(CallableMember<T, GraphicsManager>& process) {
		process.TagID = Tag;
		Scheduler->Enqueue(&process);
	}
	// Performs a general rtx task synchronously.
	template<typename T>
	inline void Perform(CallableMember<T, DXRManager>& process) {
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
	inline int Flush(int engine_mask = ENGINE_MASK_ALL) { return Scheduler->Flush (engine_mask); }

	// Sends a signal throught the specific engines and returns the Signal object with all fence values
	inline Signal SendSignal(int engine_mask) {
		return Scheduler->SendSignal(engine_mask);
	}

	// Allows the CPU to synchronizes with all pending executed command list to be finished at the GPU when some signal were achieved.
	inline void WaitFor(const Signal &s) { Scheduler->WaitFor(s); }
};

#pragma endregion

#pragma region Resource Handling

// Gets the total number of bytes required by a resource with this description
inline UINT64 GetRequiredIntermediateSize(ID3D12Device *device, const D3D12_RESOURCE_DESC &desc)
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
	ResourceWrapper(DeviceManager *manager, D3D12_RESOURCE_DESC description, ID3D12Resource *resource, D3D12_RESOURCE_STATES initialState) : manager(manager), internalResource(resource), desc(description) {
		LastUsageState = initialState; // state at creation
		D3D12_HEAP_PROPERTIES heapProp;
		D3D12_HEAP_FLAGS flags;
		resource->GetHeapProperties(&heapProp, &flags);
		if (heapProp.Type == D3D12_HEAP_TYPE_DEFAULT)
			cpu_access = CPU_ACCESS_NONE;
		else
			if (heapProp.Type == D3D12_HEAP_TYPE_UPLOAD)
				cpu_access = CPU_WRITE_GPU_READ;
			else
				cpu_access = CPU_READ_GPU_WRITE;

		subresources = desc.MipLevels * desc.DepthOrArraySize;

		pLayouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[subresources];
		pNumRows = new unsigned int[subresources];
		pRowSizesInBytes = new UINT64[subresources];
		manager->device->GetCopyableFootprints(&desc, 0, subresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &pTotalSizes);
	}

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
		if (internalResource) {
			internalResource->Release();
			internalResource = nullptr;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() {
		return internalResource->GetGPUVirtualAddress();
	}
	// Device Manager to manage this resource
	DeviceManager *manager;
	// Resource
	ID3D12Resource *internalResource;
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

	void UpdateMappedData(int position, void* data, int size) {
		D3D12_RANGE range{ };

		if (mappedData == nullptr)
		{
			mutex.Acquire();
			if (mappedData == nullptr)
				internalResource->Map(0, &range, &mappedData);
			mutex.Release();
		}

		memcpy((UINT8*)mappedData + position, (UINT8*)data, size);
	}

	byte* GetMappedDataAddress() {
		D3D12_RANGE range{ };
		if (mappedData == nullptr)
		{
			mutex.Acquire();
			if (mappedData == nullptr)
				internalResource->Map(0, &range, &mappedData);
			mutex.Release();
		}
		return (byte*)mappedData;
	}

	// Prepares this resource wrapper to have an uploading version
	void CreateForUploading() {
		if (!uploadingResourceCache) {
			mutex.Acquire();

			if (!uploadingResourceCache) {
				auto size = GetRequiredIntermediateSize(manager->device, desc);

				D3D12_RESOURCE_DESC finalDesc = { };
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

				D3D12_HEAP_PROPERTIES uploadProp;
				uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
				uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				uploadProp.VisibleNodeMask = 1;
				uploadProp.CreationNodeMask = 1;
				ID3D12Resource *res;
				manager->device->CreateCommittedResource(&uploadProp, D3D12_HEAP_FLAG_NONE, &finalDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
					IID_PPV_ARGS(&res));

				uploadingResourceCache = new ResourceWrapper(manager, this->desc, res, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
			
			mutex.Release();
		}
	}

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
	void UploadFullData(byte* data, long long dataSize, bool flipRows = false) {
		D3D12_RANGE range{ };
		
		if (mappedData == nullptr)
		{
			mutex.Acquire();
			if (mappedData == nullptr)
				internalResource->Map(0, &range, &mappedData);
			mutex.Release();
		}

		int srcOffset = 0;
		for (UINT i = 0; i < subresources; ++i)
		{
			D3D12_MEMCPY_DEST DestData = { (byte*)mappedData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
			D3D12_SUBRESOURCE_DATA subData;
			subData.pData = data + srcOffset;
			subData.RowPitch = pRowSizesInBytes[i];
			subData.SlicePitch = pRowSizesInBytes[i] * pNumRows[i];
			MemcpySubresource(&DestData, &subData, static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth, flipRows);
			srcOffset += pRowSizesInBytes[i] * pNumRows[i];
		}

		delete[] pLayouts;
		delete[] pNumRows;
		delete[] pRowSizesInBytes;
	}

	// Uploads the data of a region of a single subresource.
	template<typename T>
	void Upload(T* data, int subresource = 0, const D3D12_BOX *box = nullptr) {
		D3D12_RANGE range{ };
		if (mappedData == nullptr)
		{
			mutex.Acquire();
			if (mappedData == nullptr)
			{
				auto hr = internalResource->Map(0, &range, &mappedData);
				if (FAILED(hr))
					throw AfterShow(CA4G_Errors_Any, nullptr, hr);
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
};

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
		float4 borderColor = float4(0,0,0,0),
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

using gSampler = ManagedObject<Sampler>;

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
	void ChangeStateTo(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES dst) {
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

	void ChangeStateFromTo(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES src, D3D12_RESOURCE_STATES dst) {
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
	static gObj<ResourceView> getNullView(DeviceManager* manager, D3D12_RESOURCE_DIMENSION dimension);


protected:
	// Gets the internal resource this view is representing to.
	gObj<ResourceWrapper> resource = nullptr;
	DeviceManager *manager;

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
	ResourceView(DeviceManager *manager) :manager(manager) {

	}

	int getSRV() {
		if ((handleMask & 1) != 0)
			return srv;

		mutex.Acquire();
		if ((handleMask & 1) == 0)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC d;
			ZeroMemory(&d, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
			CreateSRVDesc(d);
			srv = manager->descriptors->cpu_csu->AllocateNewHandle();

			manager->device->CreateShaderResourceView(!resource ? nullptr : resource->internalResource, &d, manager->descriptors->cpu_csu->getCPUVersion(srv));
			handleMask |= 1;
		}
		mutex.Release();
		return srv;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getSRVHandle() {
		return manager->descriptors->cpu_csu->getCPUVersion(getSRV());
	}

	int getUAV() {
		if ((handleMask & 2) != 0)
			return uav;

		mutex.Acquire();

		if ((handleMask & 2) == 0)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC d;
			ZeroMemory(&d, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));
			CreateUAVDesc(d);
			uav = manager->descriptors->cpu_csu->AllocateNewHandle();
			manager->device->CreateUnorderedAccessView(!resource? nullptr : resource->internalResource, NULL, &d, manager->descriptors->cpu_csu->getCPUVersion(uav));
			handleMask |= 2;
		}
		mutex.Release();
		return uav;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getUAVHandle() {
		return manager->descriptors->cpu_csu->getCPUVersion(getUAV());
	}

	int getCBV() {
		if ((handleMask & 4) != 0)
			return cbv;

		mutex.Acquire();

		if ((handleMask & 4) == 0) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC d;
			ZeroMemory(&d, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
			CreateCBVDesc(d);
			cbv = manager->descriptors->cpu_csu->AllocateNewHandle();
			manager->device->CreateConstantBufferView(&d, manager->descriptors->cpu_csu->getCPUVersion(cbv));
			handleMask |= 4;
		}

		mutex.Release();
		return cbv;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getCBVHandle() {
		return manager->descriptors->cpu_csu->getCPUVersion(getCBV());
	}

	int getRTV() {
		if ((handleMask & 8) != 0)
			return rtv;

		mutex.Acquire();

		if ((handleMask & 8) == 0) {
			D3D12_RENDER_TARGET_VIEW_DESC d;
			ZeroMemory(&d, sizeof(D3D12_RENDER_TARGET_VIEW_DESC));
			CreateRTVDesc(d);
			rtv = manager->descriptors->cpu_rt->AllocateNewHandle();
			manager->device->CreateRenderTargetView(!resource ? nullptr : resource->internalResource, &d, manager->descriptors->cpu_rt->getCPUVersion(rtv));
			handleMask |= 8;
		}

		mutex.Release();
		return rtv;
	}

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

	D3D12_CPU_DESCRIPTOR_HANDLE getRTVHandle() {
		return manager->descriptors->cpu_rt->getCPUVersion(getRTV());
	}

	int getDSV() {

		if ((handleMask & 16) != 0)
			return dsv;

		mutex.Acquire();

		if ((handleMask & 16) == 0) {
			D3D12_DEPTH_STENCIL_VIEW_DESC d;
			ZeroMemory(&d, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
			CreateDSVDesc(d);
			dsv = manager->descriptors->cpu_ds->AllocateNewHandle();
			manager->device->CreateDepthStencilView(!resource ? nullptr : resource->internalResource, &d, manager->descriptors->cpu_ds->getCPUVersion(dsv));
			handleMask |= 16;
		}

		mutex.Release();
		return dsv;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getDSVHandle() {
		return manager->descriptors->cpu_ds->getCPUVersion(getDSV());
	}

public:

	~ResourceView() {
		if ((handleMask & 1) != 0)
			manager->descriptors->cpu_csu->Release(srv);
		if ((handleMask & 2) != 0)
			manager->descriptors->cpu_csu->Release(uav);
		if ((handleMask & 4) != 0)
			manager->descriptors->cpu_csu->Release(cbv);
		if ((handleMask & 8) != 0)
			manager->descriptors->cpu_rt->Release(rtv);
		if ((handleMask & 16) != 0)
			manager->descriptors->cpu_ds->Release(dsv);

		/*if (resource != nullptr) {
			resource->references--;
			if (resource->references == 0)
			{
				delete resource;
				resource = nullptr;
			}
		}*/
	}

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
		d.Format = !resource ? DXGI_FORMAT_UNKNOWN : resource->desc.Format;
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

	Buffer(ResourceWrapper* resource, int stride)
		: ResourceView(resource), ElementCount(resource->desc.Width / stride), Stride(stride)
	{
	}

	Buffer(DeviceManager* manager) : ResourceView(manager), ElementCount(1), Stride(256)
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

	Texture2D(DeviceManager* manager) : ResourceView(manager), Width(1), Height(1) {
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
	Texture2D* CreateSlice(int sliceStart = 0, int sliceCount = INT_MAX, int mipStart = 0, int mipCount = INT_MAX) {
		return new Texture2D(this->resource, this->arraySliceStart + sliceStart, sliceCount, this->mipSliceStart + mipStart, mipCount);
	}
	Texture2D* CreateSubresource(int sliceStart = 0, int mipStart = 0) {
		return CreateSlice(sliceStart, 1, mipStart, 1);
	}
	Texture2D* CreateArraySlice(int slice) {
		return CreateSlice(slice, 1);
	}
	Texture2D* CreateMipSlice(int slice) {
		return CreateSlice(arraySliceStart, arraySliceCount, slice, 1);
	}
};

#pragma endregion

#pragma region Scene Manager

class GeometriesOnGPU
{
	friend GeometryCollection;
	friend InstanceCollection;

	gObj<Buffer> bottomLevelAccDS;
	gObj<Buffer> scratchBottomLevelAccDS;
	WRAPPED_GPU_POINTER emulatedPtr;
};

// Represents a geometry collection can be used to compose scenes
// Internally manage a bottom level acceleration structure
// This structure can be lock/unlock for further modifications
class GeometryCollection {
	friend TriangleGeometryCollection;

	DeviceManager *manager;
	DXRManager *cmdList;

	GeometryCollection(DeviceManager* manager, DXRManager* cmdList) : 
		manager(manager), cmdList(cmdList),
		creating(new Creating(this)) {
	}

protected:
	list<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
public:
	class Creating {
		friend GeometryCollection;
		GeometryCollection* manager;
		Creating(GeometryCollection* manager) :manager(manager) {}
	public:
		gObj<GeometriesOnGPU> BakedGeometry();
	} * const creating;
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

	TriangleGeometryCollection(DeviceManager* manager, DXRManager* cmdList):
		GeometryCollection(manager, cmdList),
		loading(new Loader(this)), setting(new Setting(this)){}
public:
	class Loader {
		friend TriangleGeometryCollection;
		TriangleGeometryCollection *manager;
		Loader(TriangleGeometryCollection* manager) :manager(manager) {}
	public:
		void Geometry(int startVertex, int count, int transformIndex  = -1) {
			D3D12_RAYTRACING_GEOMETRY_DESC desc{ };
			desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE::D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			desc.Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
			{
				manager->boundVertices->resource->GetGPUVirtualAddress() + manager->currentVertexOffset,
				manager->boundVertices->Stride
			};
			if (transformIndex >= 0)
				desc.Triangles.Transform3x4 = manager->boundTransforms->resource->GetGPUVirtualAddress() + transformIndex * sizeof(float3x4);
			desc.Triangles.VertexFormat = manager->currentVertexFormat;
			desc.Triangles.VertexCount = count;

			manager->geometries.add(desc);
		}
	} *const loading;

	class Setting {
		friend TriangleGeometryCollection;
		TriangleGeometryCollection *manager;
		Setting(TriangleGeometryCollection *manager) :manager(manager) {}

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

	public:
		template<int N>
		void VertexBuffer(gObj<Buffer> vertices, VertexElement(&vertexLayout)[N], int positionIndex = 0) {
			UpdateLayout(vertexLayout, positionIndex);
			//vertices->ChangeStateTo(manager->cmdList->cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			manager->boundVertices = vertices;
		}
		void IndexBuffer(gObj<Buffer> indices) {
			manager->boundIndices = indices;
		}
	} * const setting;
};

class SceneOnGPU {
	friend InstanceCollection;
	friend IRTProgram;

	gObj<Buffer> topLevelAccDS;
	gObj<Buffer> scratchBuffer;
	gObj<Buffer> instancesBuffer;
	gObj<list<GeometriesOnGPU>> usedGeometries;
	WRAPPED_GPU_POINTER topLevelAccFallbackPtr;
};

// Represents a scene as a collection of instances
class InstanceCollection {
	
	friend DXRManager;

	DeviceManager *manager;
	DXRManager *cmdList;

	gObj<list<gObj<GeometriesOnGPU>>> usedGeometries;
	list<D3D12_RAYTRACING_INSTANCE_DESC> instances;
	list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC> fallbackInstances;

	InstanceCollection(DeviceManager* manager, DXRManager* cmdList):manager(manager), cmdList(cmdList),
	creating(new Creating(this)), loading(new Loading(this))
	{
		usedGeometries = new list<gObj<GeometriesOnGPU>>();
	}
public:
	class Loading {
		friend InstanceCollection;
		InstanceCollection *manager;
		Loading(InstanceCollection *manager) :manager(manager) {}
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
		void Instance(gObj<GeometriesOnGPU> geometries, UINT mask = ~0, float4x4 transform = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), UINT instanceID = INTSAFE_UINT_MAX)
		{
			manager->usedGeometries->add(geometries);

			if (manager->manager->fallbackDevice != nullptr) {
				int index = manager->instances.size();
				D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC d{ };
				d.Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				FillMat4x3(d.Transform, transform);
				d.InstanceMask = mask;
				d.InstanceID = instanceID == INTSAFE_UINT_MAX ? index : instanceID;
				d.InstanceContributionToHitGroupIndex = 0;
				d.AccelerationStructure = geometries->emulatedPtr;
				manager->fallbackInstances.add(d);
			}
			else {
				int index = manager->instances.size();
				D3D12_RAYTRACING_INSTANCE_DESC d{ };
				d.Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				FillMat4x3(d.Transform, transform);
				d.InstanceMask = mask;
				d.InstanceID = instanceID == INTSAFE_UINT_MAX ? index : instanceID;
				d.InstanceContributionToHitGroupIndex = 0;
				d.AccelerationStructure = geometries->bottomLevelAccDS->resource->GetGPUVirtualAddress();
				manager->instances.add(d);
			}
		}
	} * const loading;

	class Creating {
		friend InstanceCollection;
		InstanceCollection *manager;
		Creating(InstanceCollection *manager) :manager(manager) {}
	public:
		gObj<SceneOnGPU> BakedScene();
		
	} * const creating;
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
			layout[i] = current->createDesc(offset, size);
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
		_Description = shaderBytecode;
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
	
	virtual void __OnInitialization(DeviceManager* manager) = 0;
};

template<typename PSS, typename R, typename ...A>
unsigned long  StreamTypeBits() {
	return (1 << (int)PSS::PipelineState_Type) | StreamTypeBits<R, A...>();
}

template<typename PSS>
unsigned long  StreamTypeBits() {
	return (1 << (int)PSS::PipelineState_Type);// | StreamTypeBits<A...>();
}

class ShaderLoader {
public:
	// Use this method to load a bytecode
	static D3D12_SHADER_BYTECODE FromFile(const char* bytecodeFilePath) {
		D3D12_SHADER_BYTECODE code;
		FILE* file;
		if (fopen_s(&file, bytecodeFilePath, "rb") != 0)
		{
			throw AfterShow(CA4G_Errors_ShaderNotFound);
		}
		fseek(file, 0, SEEK_END);
		long long count;
		count = ftell(file);
		fseek(file, 0, SEEK_SET);

		byte* bytecode = new byte[count];
		int offset = 0;
		while (offset < count) {
			offset += fread_s(&bytecode[offset], min(1024, count - offset), sizeof(byte), 1024, file);
		}
		fclose(file);

		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecode;
		return code;
	}

	// Use this method to load a bytecode from a memory buffer
	static D3D12_SHADER_BYTECODE FromMemory(const byte* bytecodeData, int count) {
		D3D12_SHADER_BYTECODE code;
		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecodeData;
		return code;
	}

	// Use this method to load a bytecode from a memory buffer
	template<int count>
	static D3D12_SHADER_BYTECODE FromMemory(const byte (&bytecodeData)[count]) {
		D3D12_SHADER_BYTECODE code;
		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecodeData;
		return code;
	}

};

// -- Abstract pipeline object (can be Graphics or Compute)
// Allows creation of root signatures and leaves abstract the pipeline state object creation
template<typename ...PSS>
class PipelineBindings : public IPipelineBindings{

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

	DeviceManager *manager;

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

	void __OnInitialization(DeviceManager* manager) {
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

	void BindOnGPU(ComputeManager *manager, const list<SlotBinding> &list, int startRootParameter);

	// Creates the state object (compute or graphics)
	void CreatePSO() {

		if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE))
		{
			((RootSignatureStateManager*)setting)->SetRootSignature(rootSignature);
		}
		if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS))
		{
			RenderTargetFormatsStateManager* rtfsm = (RenderTargetFormatsStateManager*)setting;
			rtfsm->AllRenderTargets(8, DXGI_FORMAT_R8G8B8A8_UNORM);
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
			throw AfterShow(CA4G_Errors_BadPSOConstruction, TEXT(""), hr);
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
			throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error serializing root signature"));
		}

		hr = manager->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

		if (hr != S_OK)
		{
			throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error creating root signature"));
		}

		delete[] parameters;
		delete[] ranges;
	}

protected:

	PipelineBindings():setting(new PipelineStateStreamDescription())
	{
		StreamBits = StreamTypeBits<PSS...>();
	}
	
	// Binds a constant buffer view
	void CBV(int slot, gObj<Buffer>& const resource, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource , nullptr, space });
	}

	// Binds a shader resource view
	void SRV(int slot, gObj<Buffer>& const resource, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
	}

	// Binds a shader resource view
	void SRV(int slot, gObj<Texture2D>& const resource, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource , nullptr, space });
	}

	// Binds a shader resource view
	void SRV_Array(int startSlot, gObj<Texture2D>*& const resources, int &count, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, startSlot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resources, &count, space });
	}

	// Binds a sampler object view
	void SMP(int slot, gObj<Sampler> const &sampler, ShaderType type) {
		__CurrentLoadingSamplers->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, slot, D3D12_RESOURCE_DIMENSION_UNKNOWN, (void*)&sampler });
	}

	// Binds an unordered access view
	void UAV(int slot, gObj<Buffer> const &resource, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
	}

	// Binds an unordered access view
	void UAV(int slot, gObj<Texture2D> const &resource, ShaderType type, int space = 0) {
		__CurrentLoadingCSU->add(SlotBinding{ (D3D12_SHADER_VISIBILITY)type, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource, nullptr, space });
	}

	// Binds a render target view
	void RTV(int slot, gObj<Texture2D> &resource) {
		RenderTargetMax = max(RenderTargetMax, slot + 1);
		RenderTargets[slot] = &resource;
	}

	// Binds a depth-stencil buffer view
	void DBV(gObj<Texture2D> const &resource) {
		DepthBufferField = (gObj<Texture2D>*)&resource;
	}

	// Adds a static sampler to the root signature.
	void Static_SMP(int slot, const Sampler &sampler, ShaderType type, int space = 0) {
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
		_ gSet VertexShader(ShaderLoader::FromMemory(cso_DrawScreen_VS));
		_ gSet PixelShader(ShaderLoader::FromMemory(cso_DrawComplexity_PS));
		_ gSet InputLayout({
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
		_ gSet VertexShader(ShaderLoader::FromMemory(cso_DrawScreen_VS));
		_ gSet PixelShader(ShaderLoader::FromMemory(cso_DrawScreen_PS));
		_ gSet InputLayout({
				VertexElement(VertexElementType_Float, 2, "POSITION")
			});
	}

	void Globals()
	{
		RTV(0, RenderTarget);
		SRV(0, Texture, ShaderType_Pixel);
	}
};

// Allows to create a graphics pipeline state object
//class GraphicsPipelineBindings : public PipelineBindings {
//	friend GraphicsManager;
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
//	D3D12_INPUT_ELEMENT_DESC *layout;
//	DXGI_FORMAT renderTargets[8] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };
//	
//	ID3D12PipelineState *CreatePSO() {
//		ID3D12PipelineState *pso;
//		desc.pRootSignature = rootSignature;
//		desc.NumRenderTargets = RenderTargetMax;
//		for (int i = 0; i < RenderTargetMax; i++)
//			desc.RTVFormats[i] = renderTargets[i];
//		auto hr = manager->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
//
//		if (hr != S_OK) {
//			throw AfterShow(CA4G_Errors_BadPSOConstruction);
//		}
//
//		delete layout;
//		layout = nullptr;
//		return pso;
//	}
//
//	void BindOnGPU(ComputeManager *manager, const list<SlotBinding> &list, int startRootParameter);
//
//	// Setup render targets, depth buffer and globals bindings
//	void __OnSetGraphics(ComputeManager *manager);
//	
//	// Executed on every draw call, performs local bindings.
//	void __OnSetLocals(ComputeManager* manager) {
//		BindOnGPU(manager, __LocalsCSU, __GlobalsCSU.size() + __GlobalsSamplers.size());
//	}
//protected:
//
//	// Tool method for creating default rasterization state
//	static D3D12_RASTERIZER_DESC defaultRasterizerState() {
//		return D3D12_RASTERIZER_DESC{
//			D3D12_FILL_MODE_SOLID,
//			D3D12_CULL_MODE_NONE,
//			FALSE,
//			D3D12_DEFAULT_DEPTH_BIAS,
//			D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
//			D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
//			TRUE,
//			FALSE,
//			FALSE,
//			0,
//			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
//		};
//	}
//
//	// Tool method for creating default blending state
//	static D3D12_BLEND_DESC defaultBlendState() {
//		D3D12_BLEND_DESC d;
//		d.AlphaToCoverageEnable = FALSE;
//		d.IndependentBlendEnable = FALSE;
//		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
//		{
//			FALSE,FALSE,
//			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
//			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
//			D3D12_LOGIC_OP_NOOP,
//			D3D12_COLOR_WRITE_ENABLE_ALL,
//		};
//		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
//			d.RenderTarget[i] = defaultRenderTargetBlendDesc;
//		return d;
//	}
//
//	GraphicsPipelineBindings() :setting(new Setter(this)), loading(new Loading(this)) {
//		desc = { };
//		desc.RasterizerState = defaultRasterizerState();
//		desc.BlendState = defaultBlendState();
//		
//		desc.DepthStencilState.DepthEnable = FALSE;
//		desc.DepthStencilState.StencilEnable = FALSE;
//		desc.SampleMask = UINT_MAX;
//		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//		desc.SampleDesc.Count = 1;
//	}
//
//public:
//
//	// Loading module for setup vertex and pixel shaders.
//	class Loading {
//		friend GraphicsPipelineBindings;
//		GraphicsPipelineBindings *parent;
//		Loading(GraphicsPipelineBindings *parent) :parent(parent) {}
//	public:
//		// Loads a vertex shader bytecode and set as vertex shader in this pipeline state object.
//		GraphicsPipelineBindings* VertexShader(const char* bytecodeFilePath) {
//			parent->LoadByteCode(bytecodeFilePath, parent->desc.VS);
//			parent->Using_VS = true;
//			return parent;
//		}
//		// Loads a pixel shader bytecode and set as vertex shader in this pipeline state object.
//		GraphicsPipelineBindings* PixelShader(const char* bytecodeFilePath) {
//			parent->LoadByteCode(bytecodeFilePath, parent->desc.PS);
//			parent->Using_PS = true;
//			return parent;
//			// Loads a vertex shader bytecode and set as vertex shader in this pipeline state object.
//		}
//	} *const loading;
//
//	// Setter module for setup depth testing, rasterization state and input layout.
//	class Setter {
//		friend GraphicsPipelineBindings;
//		GraphicsPipelineBindings *pipeline;
//		Setter(GraphicsPipelineBindings* pipeline) :pipeline(pipeline) {
//		}
//	public:
//		Setter* DepthTest(bool depthEnable = true, bool depthWrite = true, D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_LESS_EQUAL) {
//			D3D12_DEPTH_STENCIL_DESC &desc = pipeline->desc.DepthStencilState;
//			desc.DepthEnable = depthEnable;
//			desc.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
//			desc.DepthFunc = comparison;
//			return this;
//		}
//
//		Setter* Rasterizer(D3D12_CULL_MODE cull = D3D12_CULL_MODE_NONE, D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID, bool conservative = false) {
//			D3D12_RASTERIZER_DESC &desc = pipeline->desc.RasterizerState;
//			desc.FillMode = fillMode;
//			desc.CullMode = cull;
//			desc.ConservativeRaster = conservative ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
//			return this;
//		}
//
//		Setter* InputLayout(std::initializer_list<VertexElement> elements) {
//			if (pipeline->layout != nullptr)
//				delete pipeline->layout;
//
//			pipeline->layout = new D3D12_INPUT_ELEMENT_DESC[elements.size()];
//			int offset = 0;
//			auto current = elements.begin();
//			for (int i = 0; i < elements.size(); i++)
//			{
//				int size = 0;
//				pipeline->layout[i] = current->createDesc(offset, size);
//				offset += size;
//				current++;
//			}
//			pipeline->desc.InputLayout.NumElements = elements.size();
//			pipeline->desc.InputLayout.pInputElementDescs = pipeline->layout;
//			return this;
//		}
//
//		void Close() {
//			pipeline->Close();
//		}
//	} *const setting;
//
//	// Opens this graphics pipeline object. Reseting pipeline state object and 
//	// leaving this object ready to be setup.
//	GraphicsPipelineBindings* Open() {
//		if (pso != nullptr) {
//			//pso->Release();
//			delete pso;
//		}
//		pso = nullptr;
//		return this;
//	}
//};

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

	ID3D12RootSignature *rootSignature;
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
		wcscat(label, TEXT("HITGROUP_"));
		wsprintf(&label[7], L"%d",
			ID);

		ID++;

		return label;
	}

	AnyHitHandle anyHit;
	ClosestHitHandle closestHit;
	IntersectionHandle intersection;

	HitGroupHandle(ClosestHitHandle closestHit, AnyHitHandle anyHit, IntersectionHandle intersection) 
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
		void Shader(RayGenerationHandle &handle, LPCWSTR shader) {
			manager->exports.add({ shader });
			handle = RayGenerationHandle(shader);
		}
		void Shader(MissHandle &handle, LPCWSTR shader) {
			manager->exports.add({ shader });
			handle = MissHandle(shader);
		}
		void Shader(AnyHitHandle &handle, LPCWSTR shader) {
			manager->exports.add({ shader });
			handle = AnyHitHandle(shader);
		}
		void Shader(ClosestHitHandle &handle, LPCWSTR shader) {
			manager->exports.add({ shader });
			handle = ClosestHitHandle(shader);
		}
		void Shader(IntersectionHandle &handle, LPCWSTR shader) {
			manager->exports.add({ shader });
			handle = IntersectionHandle(shader);
		}
	} * const loading;
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
	list<HitGroupHandle> hitGroups;
	// Gets the list of all associations between shaders and global bindings
	list<LPCWSTR> associationsToGlobal;
	// Gets the list of all associations between shaders and raygen local bindings
	list<LPCWSTR> associationsToRayGenLocals;
	// Gets the list of all associations between shaders and miss local bindings
	list<LPCWSTR> associationsToMissLocals;
	// Gets the list of all associations between shaders and hitgroup local bindings
	list<LPCWSTR> associationsToHitGroupLocals;
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
	int MaxGroups = 1;
	// Gets the maximum number of miss programs that will be setup before any
	// single dispatch rays
	int MaxMiss = 1;
	DeviceManager *manager;

	void UpdateRayGenLocals(DXRManager* cmdList, RayGenerationHandle shader);
	void UpdateMissLocals(DXRManager* cmdList, MissHandle shader, int index);
	void UpdateHitGroupLocals(DXRManager* cmdList, HitGroupHandle shader, int index);

	void BindOnGPU(DXRManager *manager, gObj<BindingsHandle> globals);

	void BindLocalsOnShaderTable(gObj<BindingsHandle> locals, byte* shaderRecordData);
};

// Represents a raytracing program with shader tables and local bindings management
template<typename R>
class RTProgram : IRTProgram {
	friend RTPipelineManager;
	friend DXRManager;
	
	R* _rt_manager;

	void __OnInitialization(DeviceManager *manager, RTPipelineManager* rtManager) {
		this->manager = manager;
		this->_rt_manager = (R*)rtManager;
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

	// Creates the root signature after closing
	ID3D12RootSignature* CreateRootSignature(gObj<BindingsHandle> bindings, int &size) {
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

		ID3DBlob *signatureBlob;
		ID3DBlob *signatureErrorBlob;

		ID3D12RootSignature *rootSignature;

		if (manager->fallbackDevice != nullptr) // will use fallback for DX RT support
		{
			auto hr = manager->fallbackDevice->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
			if (hr != S_OK)
				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error serializing root signature"));
			hr = manager->fallbackDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
			if (hr != S_OK)
				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error creating root signature"));
		}
		else
		{
			auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
			if (hr != S_OK)
				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error serializing root signature"));

			hr = manager->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
			if (hr != S_OK)
				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error creating root signature"));
		}

		delete[] parameters;

		return rootSignature;
	}

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
		b.DescriptorData.ptrToResourceViewArray = (void*)&resource;
		b.DescriptorData.ptrToCount = (void*)&count;
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

		Static_Samplers[StaticSamplerMax] = desc;
		StaticSamplerMax++;
	}

#pragma endregion

	class Loader {
		RTProgram *manager;
	public:
		Loader(RTProgram *manager) :manager(manager) { }

		void Shader(const RayGenerationHandle &handle) {
			manager->associationsToGlobal.add(handle.shaderHandle);
			manager->associationsToRayGenLocals.add(handle.shaderHandle);
		}

		void Shader(const MissHandle &handle) {
			manager->associationsToGlobal.add(handle.shaderHandle);
			manager->associationsToMissLocals.add(handle.shaderHandle);
		}

	} * const loading;

	class Creating {
		RTProgram *manager;
	public:
		Creating(RTProgram *manager) : manager(manager){
		}

		void HitGroup(HitGroupHandle &handle, ClosestHitHandle &closest, const AnyHitHandle &anyHit, const IntersectionHandle &intersection) {
			handle = HitGroupHandle(closest, anyHit, intersection);
			manager->hitGroups.add(handle);
			manager->associationsToGlobal.add(handle.shaderHandle);
			manager->associationsToHitGroupLocals.add(handle.shaderHandle);
		}
	} * const creating;

	class Setter {
		RTProgram *manager;
	public:
		Setter(RTProgram *manager) :manager(manager) {}
		void Payload(int sizeInBytes) {
			manager->PayloadSize = sizeInBytes;
		}
		void StackSize(int maxDeep) {
			manager->StackSize = maxDeep;
		}
		void Attribute(int sizeInBytes) {
			manager->AttributesSize = sizeInBytes;
		}
	} * const setting;

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
	DeviceManager *manager;

	ID3D12RaytracingFallbackStateObject* fbso;
	ID3D12StateObject *so;

	// Will be executed when this pipeline manager were loaded
	void __OnInitialization(DeviceManager *manager) {
		this->manager = manager;
		Setup();
	}

	void Close() {
		// TODO: Create the so

#pragma region counting states
		int count = 0;

		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
		count += loadedLibraries.size();

		int maxAttributes = 2 * 4;
		int maxPayload = 3 * 4;
		int maxStackSize = 1;

		for (int i = 0; i < loadedPrograms.size(); i++)
		{
			gObj<IRTProgram> program = loadedPrograms[i];

			maxAttributes = max(maxAttributes, program->AttributesSize);
			maxPayload = max(maxPayload, program->PayloadSize);
			maxStackSize = max(maxStackSize, program->StackSize);

			// Global root signature
			if (!program->globals.isNull())
				count++;
			// Local raygen root signature
			if (!program->raygen_locals.isNull())
				count++;
			// Local miss root signature
			if (!program->miss_locals.isNull())
				count++;
			// Local hitgroup root signature
			if (!program->hitGroup_locals.isNull())
				count++;

			// Associations to global root signature
			if (program->associationsToGlobal.size() > 0)
				count++;
			// Associations to raygen local root signature
			if (program->associationsToRayGenLocals.size() > 0)
				count++;
			// Associations to miss local root signature
			if (program->associationsToMissLocals.size() > 0)
				count++;
			// Associations to hitgroup local root signature
			if (program->associationsToHitGroupLocals.size() > 0)
				count++;
			// 1 x each hit group
			count += program->hitGroups.size();
		}

		// 1 x shader config
		count++;
		// 1 x pipeline config
		count++;

#pragma endregion

		AllocateStates(count);

#pragma region Fill States

		int index = 0;
		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
		for (int i = 0; i < loadedLibraries.size(); i++)
			SetDXIL(index++, loadedLibraries[i]->bytecode, loadedLibraries[i]->exports);

		D3D12_STATE_SUBOBJECT* globalRS;
		D3D12_STATE_SUBOBJECT* localRayGenRS;
		D3D12_STATE_SUBOBJECT* localMissRS;
		D3D12_STATE_SUBOBJECT* localHitGroupRS;

		for (int i = 0; i < loadedPrograms.size(); i++)
		{
			gObj<IRTProgram> program = loadedPrograms[i];

			// Global root signature
			if (!program->globals.isNull())
			{
				globalRS = &dynamicStates[index];
				SetGlobalRootSignature(index++, program->globals->rootSignature);
			}
			// Local raygen root signature
			if (!program->raygen_locals.isNull())
			{
				localRayGenRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->raygen_locals->rootSignature);
			}
			// Local miss root signature
			if (!program->miss_locals.isNull())
			{
				localMissRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->miss_locals->rootSignature);
			}
			// Local hitgroup root signature
			if (!program->hitGroup_locals.isNull())
			{
				localHitGroupRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->hitGroup_locals->rootSignature);
			}

			for (int j = 0; j < program->hitGroups.size(); j++)
			{
				auto hg = program->hitGroups[j];
				if (hg.intersection.IsNull())
					SetTriangleHitGroup(index++, hg.shaderHandle, hg.anyHit.shaderHandle, hg.closestHit.shaderHandle);
				else
					SetProceduralGeometryHitGroup(index++, hg.shaderHandle, hg.anyHit.shaderHandle, hg.closestHit.shaderHandle, hg.intersection.shaderHandle);
			}

			// Associations to global root signature
			if (program->associationsToGlobal.size() > 0)
				SetExportsAssociations(index++, globalRS, program->associationsToGlobal);
			// Associations to raygen local root signature
			if (!program->raygen_locals.isNull() && program->associationsToRayGenLocals.size() > 0)
				SetExportsAssociations(index++, localRayGenRS, program->associationsToRayGenLocals);
			// Associations to miss local root signature
			if (!program->miss_locals.isNull() && program->associationsToMissLocals.size() > 0)
				SetExportsAssociations(index++, localMissRS, program->associationsToMissLocals);
			// Associations to hitgroup local root signature
			if (!program->hitGroup_locals.isNull() && program->associationsToHitGroupLocals.size() > 0)
				SetExportsAssociations(index++, localHitGroupRS, program->associationsToHitGroupLocals);
		}

		// 1 x shader config
		SetRTSizes(index++, maxAttributes, maxPayload);
		SetMaxRTRecursion(index++, maxStackSize);

#pragma endregion

		// Create so
		D3D12_STATE_OBJECT_DESC soDesc = { };
		soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		soDesc.NumSubobjects = index;
		soDesc.pSubobjects = this->dynamicStates;

		if (manager->fallbackDevice != nullptr) // emulating with fallback device
		{
			auto hr = manager->fallbackDevice->CreateStateObject(&soDesc, IID_PPV_ARGS(&fbso));
			if (FAILED(hr))
				throw AfterShow(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}
		else {
			auto hr = manager->device->CreateStateObject(&soDesc, IID_PPV_ARGS(&so));
			if (FAILED(hr))
				throw AfterShow(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}
	}

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
	void __OnSet(DXRManager *cmdManager);

protected:


	class Loader {
		RTPipelineManager *manager;
	public:
		Loader(RTPipelineManager *manager):manager(manager) {
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
	} * const loading;

	virtual void Setup() = 0;

	RTPipelineManager() : loading(new Loader(this))
	{
	}
};


// Represents a pipeline bindings object for rt engines
//class RTPipelineBindings : public IStateBindings, virtual DynamicStateBindings,
//	DXILManager,
//	GlobalRootSignatureManager,
//	LocalRootSignatureManager,
//	HitGroupManager,
//	RTShaderConfig,
//	RTPipelineConfig {
//
//	friend DXRManager;
//
//	// Deferred association struct. Used to "remember" 
//	// needed associations for the state object construction on close
//	struct DeferredAssociation {
//		int SubojectOffset;
//		list<LPCWSTR> exports;
//	};
//
//	// Bytecode of the last DXIL library loaded
//	D3D12_SHADER_BYTECODE currentBytecode = { };
//	// List with the exports for the last DXIL library loaded
//	list<D3D12_EXPORT_DESC> currentExports;
//	// List of every loaded library descriptions
//	list<D3D12_DXIL_LIBRARY_DESC> loadedLibraries;
//	// Global list of every exported list to have everything saved 
//	// in memory when state object is created
//	list<list<D3D12_EXPORT_DESC>> allExports;
//
//	// Current global bindings object loaded to associate with every
//	// shader export next
//	gObj<BindingsHandle> currentGlobals;
//	// Current local bindings object loaded to associate with every
//	// shader export next
//	gObj<BindingsHandle> currentLocals;
//
//	// List of exports will be associated to locals bindings
//	list<LPCWSTR> localsExports;
//	// List of exports will be associated to global bindings
//	list<LPCWSTR> globalsExports;
//
//	// List of deferred associations
//	list<DeferredAssociation> associations;
//
//	// Internal device manager for DX12 functionalities
//	DeviceManager* manager;
//
//	// Used to collect static samplers
//	D3D12_STATIC_SAMPLER_DESC Static_Samplers[32];
//	// Gets or sets the maxim sampler slot used
//	int StaticSamplerMax = 0;
//	// Use to change between global and local list during collecting
//	list<SlotBinding> *__CurrentLoadingCSU;
//	// Use to change between global and local list during collecting
//	list<SlotBinding> *__CurrentLoadingSamplers;
//
//	void AppendLibraryIfAny() {
//		AppendGlobalSignatureIfNotEmpty();
//		if (currentBytecode.pShaderBytecode != nullptr) {
//			// save for future memory accessing
//			allExports.add(currentExports.clone());
//			
//			this->AddDXIL(currentBytecode, allExports.last());
//
//			currentExports.reset();
//			currentBytecode = { };
//		}
//	}
//
//	void Close() {
//		AppendLibraryIfAny();
//
//		SetMaxRTRecursion(setting->maxTraceRecursion);
//		SetRTTypes(setting->attributeSize, setting->maxPayloadSize);
//
//		// Create so
//		D3D12_STATE_OBJECT_DESC soDesc = { };
//		soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
//		soDesc.NumSubobjects = this->dynamicStates.size();
//		soDesc.pSubobjects = &this->dynamicStates.first();
//
//		if (manager->fallbackDevice != nullptr) // emulating with fallback device
//		{
//			auto hr = manager->fallbackDevice->CreateStateObject(&soDesc, IID_PPV_ARGS(&fbso));
//			if (FAILED(hr))
//				throw AfterShow(CA4G_Errors_BadPSOConstruction, nullptr, hr);
//		}
//		else {
//			auto hr = manager->device->CreateStateObject(&soDesc, IID_PPV_ARGS(&so));
//			if (FAILED(hr))
//				throw AfterShow(CA4G_Errors_BadPSOConstruction, nullptr, hr);
//		}
//	}
//
//	byte* GetShaderIdentifier(ProgramHandle shader) {
//		if (manager->fallbackDevice != nullptr)
//			return (byte*)fbso->GetShaderIdentifier(shader.shaderHandle);
//		else
//			return (byte*)((ID3D12StateObjectPropertiesPrototype*)so)->GetShaderIdentifier(shader.shaderHandle);
//	}
//
//	gObj<Buffer> raygenShaderTable;
//	gObj<Buffer> missShaderTable;
//	gObj<Buffer> hitgroupShaderTable;
//
//	void CommitLocalsFor(ProgramHandle handle, gObj<Buffer> &shaderTable, int index) {
//		byte shaderRecord[4096]; // 4K limit of sh records
//		int shaderIdentifierSize = manager->fallbackDevice != nullptr ? manager->fallbackDevice->GetShaderIdentifierSize() : D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
//		memcpy(shaderRecord, GetShaderIdentifier(handle), shaderIdentifierSize);
//
//		shaderTable->UpdateShaderRecord(index, shaderRecord);
//	}
//
//	void CommitLocalsFor(RayGenerationHandle handle, int index) {
//		CommitLocalsFor(handle, raygenShaderTable, index);
//	}
//	void CommitLocalsFor(HitGroupHandle handle, int index) {
//		CommitLocalsFor(handle, hitgroupShaderTable, index);
//	}
//	void CommitLocalsFor(MissHandle handle, int index) {
//		CommitLocalsFor(handle, missShaderTable, index);
//	}
//
//	void AppendLocalSignatureIfNotEmpty() {
//		if (localsExports.size() > 0) {
//			AddLocalRootSignature(currentLocals->rootSignature);
//			auto suboject = &dynamicStates.last();
//			associations.add({ (byte*)suboject - (byte*)&dynamicStates.first(), localsExports.clone() });
//			localsExports.reset();
//		}
//	}
//
//	void AppendGlobalSignatureIfNotEmpty() {
//		AppendLocalSignatureIfNotEmpty();
//
//		if (globalsExports.size() > 0) {
//			AddGlobalRootSignature(currentGlobals->rootSignature);
//			auto suboject = &dynamicStates.last();
//			associations.add({ (byte*)suboject - (byte*)&dynamicStates.first(), globalsExports.clone() });
//			globalsExports.reset();
//		}
//	}
//
//	void AppendDeferredAssociations() {
//		int currentSize = dynamicStates.size();
//		for (int i = 0; i < associations.size(); i++)
//			dynamicStates.add({ }); // enlarge dynamic state all necessary remain subobjects
//		for (int i = 0; i < associations.size(); i++) {
//			DeferredAssociation& a = associations[i];
//			dynamicStates[i + currentSize].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
//
//			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION();
//			desc->NumExports = a.exports.size();
//			desc->pExports = &a.exports.first();
//			desc->pSubobjectToAssociate = &dynamicStates.first() + a.SubojectOffset;
//			dynamicStates[i + currentSize].pDesc = desc;
//		}
//	}
//
//	// Creates the root signature after closing
//	ID3D12RootSignature* CreateRootSignature(list<SlotBinding> &csu, list<SlotBinding> &samplers, bool isLocal) {
//		D3D12_ROOT_PARAMETER * parameters = new D3D12_ROOT_PARAMETER[csu.size() + samplers.size()];
//		D3D12_DESCRIPTOR_RANGE * ranges = new D3D12_DESCRIPTOR_RANGE[csu.size() + samplers.size()];
//		int index = 0;
//		for (int i = 0; i < csu.size(); i++)
//
//			//if (!csu[i].direct)
//		{
//			D3D12_DESCRIPTOR_RANGE range = { };
//			range.RangeType = csu[i].type;
//			range.BaseShaderRegister = csu[i].slot;
//			range.NumDescriptors = csu[i].ptrToCount == nullptr ? 1 : -1;
//			range.RegisterSpace = 0;
//			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
//			//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
//			ranges[index] = range;
//
//			D3D12_ROOT_PARAMETER p = { };
//			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//			p.DescriptorTable.NumDescriptorRanges = 1;
//			p.DescriptorTable.pDescriptorRanges = &ranges[index];
//			p.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//			parameters[index] = p;
//
//			index++;
//		}
//		//else
//		//{
//		//	D3D12_ROOT_PARAMETER p = { };
//		//	p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
//		//	//p.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
//		//	p.Descriptor.RegisterSpace = csu[i].space;
//		//	p.Descriptor.ShaderRegister = csu[i].slot;
//		//	p.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//		//	parameters[index] = p;
//
//		//	index++;
//		//}
//
//		for (int i = 0; i < samplers.size(); i++)
//		{
//			D3D12_DESCRIPTOR_RANGE range = { };
//			range.RangeType = samplers[i].type;
//			range.BaseShaderRegister = samplers[i].slot;
//			range.NumDescriptors = samplers[i].ptrToCount == nullptr ? 1 : -1;
//			range.RegisterSpace = 0;
//			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
//			//range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
//			ranges[index] = range;
//
//			D3D12_ROOT_PARAMETER p = { };
//			p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//			p.DescriptorTable.NumDescriptorRanges = 1;
//			p.DescriptorTable.pDescriptorRanges = &ranges[index];
//			p.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//			parameters[index] = p;
//
//			index++;
//		}
//
//		D3D12_ROOT_SIGNATURE_DESC desc;
//		desc.pParameters = parameters;
//		desc.NumParameters = index;
//		desc.pStaticSamplers = Static_Samplers;
//		desc.NumStaticSamplers = StaticSamplerMax;
//		desc.Flags = isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
//
//		ID3DBlob *signatureBlob;
//		ID3DBlob *signatureErrorBlob;
//
//		ID3D12RootSignature *rootSignature;
//
//		if (manager->fallbackDevice != nullptr) // will use fallback for DX RT support
//		{
//			auto hr = manager->fallbackDevice->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
//			if (hr != S_OK)
//				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error serializing root signature"));
//			hr = manager->fallbackDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
//			if (hr != S_OK)
//				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error creating root signature"));
//		}
//		else
//		{
//			auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrorBlob);
//			if (hr != S_OK)
//				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error serializing root signature"));
//
//			hr = manager->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
//			if (hr != S_OK)
//				throw AfterShow(CA4G_Errors_BadSignatureConstruction, TEXT("Error creating root signature"));
//		}
//
//		delete[] parameters;
//		delete[] ranges;
//
//		return rootSignature;
//	}
//
//	void __OnSet(DXRManager * manager);
//
//protected:
//
//	virtual void Setup() = 0;
//
//	typedef void (RTPipelineBindings::*BindingsMethod)();
//
//	RTPipelineBindings() : setting(new Setter()), loading(new Loading(this)), creating(new Creating(this)) {
//	}
//
//	class Loading {
//		friend RTPipelineBindings;
//		RTPipelineBindings* manager;
//		Loading(RTPipelineBindings *manager) :manager(manager) {}
//
//		void DeclareExport(LPCWSTR name) {
//			manager->currentExports.add({ name });
//			manager->globalsExports.add(name);
//			manager->localsExports.add(name);
//		}
//
//		void GetBindingsFor(BindingsMethod bindings, list<SlotBinding> &csu, list<SlotBinding> &samplers) {
//			csu.reset();
//			samplers.reset();
//
//			manager->__CurrentLoadingCSU = &csu;
//			manager->__CurrentLoadingSamplers = &samplers;
//
//			auto instance = this->manager;
//
//			(instance->*bindings)();
//		}
//
//	public:
//
//		void Library(D3D12_SHADER_BYTECODE bytecode) {
//			manager->AppendLibraryIfAny();
//			manager->currentBytecode = bytecode;
//		}
//
//		void Globals(BindingsMethod globals) {
//			manager->AppendGlobalSignatureIfNotEmpty();
//			gObj<BindingsHandle> handle = new BindingsHandle();
//			handle->isLocal = false;
//			GetBindingsFor(globals, handle->csuBindings, handle->samplerBindings);
//			handle->rootSignature = manager->CreateRootSignature(handle->csuBindings, handle->samplerBindings, handle->isLocal);
//			manager->currentGlobals = handle;
//		}
//
//		void Locals(BindingsMethod locals) {
//			manager->AppendLocalSignatureIfNotEmpty();
//			gObj<BindingsHandle> handle = new BindingsHandle();
//			handle->isLocal = true;
//			GetBindingsFor(locals, handle->csuBindings, handle->samplerBindings);
//			manager->currentLocals = handle;
//		}
//
//		void Shader(RayGenerationHandle &raygen, LPCWSTR name) {
//			raygen = RayGenerationHandle(name);
//			DeclareExport(name);
//		}
//		void Shader(MissHandle &miss, LPCWSTR name) {
//			miss = MissHandle(name);
//			DeclareExport(name);
//		}
//		void Shader(AnyHitHandle &anyHit, LPCWSTR name) {
//			anyHit = AnyHitHandle(name);
//			DeclareExport(name);
//		}
//		void Shader(ClosestHitHandle &closestHit, LPCWSTR name) {
//			closestHit = ClosestHitHandle(name);
//			DeclareExport(name);
//		}
//		void Shader(IntersectionHandle &intersection, LPCWSTR name) {
//			intersection = IntersectionHandle(name);
//			DeclareExport(name);
//		}
//		void HitGroup(HitGroupHandle &hitGroup,
//			const ClosestHitHandle &closestHit,
//			const AnyHitHandle &anyHit) {
//			hitGroup = HitGroupHandle(closestHit, anyHit, IntersectionHandle::NULL_INTERSECTION);
//			manager->AddTriangleHitGroup(hitGroup.shaderHandle,
//				anyHit.shaderHandle, closestHit.shaderHandle);
//			DeclareExport(hitGroup.shaderHandle);
//		}
//	} *const loading;
//
//	class Creating {
//		friend RTPipelineBindings;
//		RTPipelineBindings* manager;
//		Creating(RTPipelineBindings *manager) :manager(manager) {}
//	public:
//	} *const creating;
//
//	#pragma region Binding Methods
//	// Binds a constant buffer view
//	void CBV(int slot, gObj<Buffer>& const resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource , nullptr, space });
//	}
//
//	// Binds a shader resource view
//	void SRV(int slot, gObj<Buffer>& const resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
//	}
//
//	// Binds a acceleration structure to the pipeline
//	void ASV(int slot, gObj<Buffer>& const resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space, true });
//	}
//
//	// Binds a shader resource view
//	void SRV(int slot, gObj<Texture2D>& const resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource , nullptr, space });
//	}
//
//	// Binds a shader resource view
//	void SRV_Array(int startSlot, gObj<Texture2D>*& const resources, int &count, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, startSlot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resources, &count, space });
//	}
//
//	// Binds a sampler object view
//	void SMP(int slot, gObj<Sampler> const &sampler) {
//		__CurrentLoadingSamplers->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, slot, D3D12_RESOURCE_DIMENSION_UNKNOWN, (void*)&sampler });
//	}
//
//	// Binds an unordered access view
//	void UAV(int slot, gObj<Buffer> const &resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_BUFFER, (void*)&resource, nullptr, space });
//	}
//
//	// Binds an unordered access view
//	void UAV(int slot, gObj<Texture2D> const &resource, int space = 0) {
//		__CurrentLoadingCSU->add(SlotBinding{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slot, D3D12_RESOURCE_DIMENSION_TEXTURE2D, (void*)&resource, nullptr, space });
//	}
//
//	// Adds a static sampler to the root signature.
//	void Static_SMP(int slot, const Sampler &sampler, int space = 0) {
//		D3D12_STATIC_SAMPLER_DESC desc = { };
//		desc.AddressU = sampler.AddressU;
//		desc.AddressV = sampler.AddressV;
//		desc.AddressW = sampler.AddressW;
//		desc.BorderColor =
//			!any(sampler.BorderColor - float4(0, 0, 0, 0)) ?
//			D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
//			!any(sampler.BorderColor - float4(0, 0, 0, 1)) ?
//			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK :
//			D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
//		desc.ComparisonFunc = sampler.ComparisonFunc;
//		desc.Filter = sampler.Filter;
//		desc.MaxAnisotropy = sampler.MaxAnisotropy;
//		desc.MaxLOD = sampler.MaxLOD;
//		desc.MinLOD = sampler.MinLOD;
//		desc.MipLODBias = sampler.MipLODBias;
//		desc.RegisterSpace = space;
//		desc.ShaderRegister = slot;
//		desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
//
//		Static_Samplers[StaticSamplerMax] = desc;
//		StaticSamplerMax++;
//	}
//
//	#pragma endregion
//
//	class Setter {
//		friend RTPipelineBindings;
//
//		int maxTraceRecursion = 8;
//		int maxPayloadSize = 32;
//		int attributeSize = 16;
//	public:
//		void MaxTraceRecursion(int stacking) {
//			maxTraceRecursion = stacking;
//		}
//		void MaxPayloadSize(int size) {
//			maxPayloadSize = size;
//		}
//		void AttributeSize(int size) {
//			attributeSize = size;
//		}
//	} *const setting;
//
//};

#pragma endregion

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

	ID3D12Device* getInternalDevice();

	bool EquivalentDesc(const D3D12_RESOURCE_DESC &d1, const D3D12_RESOURCE_DESC &d2) {
		return d1.Width == d2.Width && d1.Height == d2.Height && d1.DepthOrArraySize == d2.DepthOrArraySize;
	}


	// Use this command list to copy from one resource to another
	void __All(gObj<ResourceWrapper> &dst, gObj<ResourceWrapper> &src);

protected:

	list<D3D12_CPU_DESCRIPTOR_HANDLE> srcDescriptors;
	list<D3D12_CPU_DESCRIPTOR_HANDLE> dstDescriptors;
	list<unsigned int> dstDescriptorRangeLengths;

	ID3D12GraphicsCommandList5 *cmdList;
	DeviceManager* manager;

	CommandListManager(DeviceManager* manager, ID3D12GraphicsCommandList5 *cmdList) :
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
		ID3D12GraphicsCommandList *cmdList;
		CommandListManager *manager;
	public:
		Clearing(CommandListManager *manager, ID3D12GraphicsCommandList *cmdList) :manager(manager),cmdList(cmdList) {}

		inline Clearing* UAV(gObj<ResourceView> uav, const FLOAT values[4]) {
			cmdList->ClearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE{}, uav->getUAVHandle(), uav->resource->internalResource, values, 0, nullptr);
			return this;
		}
		inline Clearing* UAV(gObj<ResourceView> uav, const float4 &value) {
			float v[4]{ value.x, value.y, value.z, value.w };
			cmdList->ClearUnorderedAccessViewFloat(
				this->manager->manager->descriptors->gpu_csu->getGPUVersion(uav->getUAV()), 
				uav->getUAVHandle(), uav->resource->internalResource, v, 0, nullptr);
			return this;
		}
		inline Clearing* UAV(gObj<ResourceView> uav, const unsigned int &value) {
			unsigned int v[4]{ value, value, value, value };
			uav->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ClearUnorderedAccessViewUint(
				this->manager->manager->descriptors->gpu_csu->getGPUVersion(uav->getUAV()),
				uav->getUAVHandle(), uav->resource->internalResource, v, 0, nullptr);
			return this;
		}
		inline Clearing* UAV(gObj<ResourceView> uav, const uint4 &value) {
			unsigned int v[4]{ value.x, value.y, value.z, value.w };
			cmdList->ClearUnorderedAccessViewUint(
				this->manager->manager->descriptors->gpu_csu->getGPUVersion(uav->getUAV()),
				uav->getUAVHandle(), uav->resource->internalResource, v, 0, nullptr);
			return this;
		}
		inline Clearing* UAV(gObj<ResourceView> uav, const unsigned int values[4]) {
			cmdList->ClearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE{}, uav->getUAVHandle(), uav->resource->internalResource, values, 0, nullptr);
			return this;
		}
		inline Clearing* RT(gObj<Texture2D> rt, const FLOAT values[4]) {
			cmdList->ClearRenderTargetView(rt->getRTVHandle(), values, 0, nullptr);
			return this;
		}
		inline Clearing* RT(gObj<Texture2D> rt, const float4 &value) {
			float v[4]{ value.x, value.y, value.z, value.w };
			cmdList->ClearRenderTargetView(rt->getRTVHandle(), v, 0, nullptr);
			return this;
		}
		// Clears the render target accessed by the Texture2D View with a specific float3 value
		inline Clearing* RT(gObj<Texture2D> rt, const float3 &value) {
			float v[4]{ value.x, value.y, value.z, 1.0f };
			cmdList->ClearRenderTargetView(rt->getRTVHandle(), v, 0, nullptr);
			return this;
		}
		inline Clearing* Depth(gObj<Texture2D> depthStencil, float depth = 1.0f) {
			cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
			return this;
		}
		inline Clearing* DepthStencil(gObj<Texture2D> depthStencil, float depth, UINT8 stencil) {
			cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
			return this;
		}
		inline Clearing* Stencil(gObj<Texture2D> depthStencil, UINT8 stencil) {
			cmdList->ClearDepthStencilView(depthStencil->getDSVHandle(), D3D12_CLEAR_FLAG_STENCIL, 0, stencil, 0, nullptr);
			return this;
		}
	}
	*const clearing;

	// Allows access to all copying functions from and to Resources
	class Copying {
		ID3D12GraphicsCommandList *cmdList;
		CommandListManager *manager;

		// Data is given by a reference using a pointer
		template<typename T>
		Copying* GenericCopy(gObj<ResourceView> dst, T* data, const D3D12_BOX *box = nullptr) {
			if (dst->arraySliceCount > 1 || dst->mipSliceCount > 1)
				throw AfterShow(CA4G_Errors_Invalid_Operation, TEXT("Can not update a region of a resource of multiple subresources"));

			int subresource = dst->mipSliceStart + dst->resource->desc.DepthOrArraySize * dst->arraySliceStart;

			if (dst->resource->cpu_access == CPU_WRITE_GPU_READ)
				dst->resource->Upload(data, subresource, box);
			else
			{
				dst->resource->CreateForUploading();
				dst->resource->uploadingResourceCache->Upload(data, subresource, box);
				dst->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
				manager->__All(dst->resource, dst->resource->uploadingResourceCache);
				//dst->ChangeStateTo(cmdList, D3D12_RESOURCE_STATE_GENERIC_READ);
			}
			return this;
		}

	public:
		Copying(CommandListManager *manager, ID3D12GraphicsCommandList *cmdList) : manager(manager), cmdList(cmdList) {}

		template<typename T>
		Copying* PtrDataToSubresource(gObj<Texture2D> dst, int mip, int slice, T* data, const D3D12_BOX *box = nullptr) {
			Texture2D* subresource = dst->CreateSlice(slice, 1, mip, 1);
			GenericCopy(subresource, data, box);
			delete subresource;
			return this;
		}

		template<typename T>
		Copying* PtrData(gObj<Buffer> dst, T* data, const D3D12_BOX *box = nullptr) {
			return GenericCopy(dst, data, box);
		}

		// Data is given by an initializer list
		template<typename T>
		Copying* ListData(gObj<Buffer> dst, std::initializer_list<T> data) {
			return GenericCopy(dst, data.begin());
		}

		// Data is given expanded in a value object
		template<typename T>
		Copying* ValueData(gObj<Buffer> dst, const T &data) {
			return GenericCopy(dst, &data);
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
		void FromData(gObj<Texture2D> &texture, TextureData* data);

		// Creates a 2D texture from a specific file
		// Supported formats, dds, any wic image and tga.
		void FromFile(gObj<Texture2D> &texture, const char* filePath) {
			TextureData* data = TextureData::LoadFromFile(filePath);
			FromData(texture, data);
			delete data;
		}

	}* const loading;
};

// Allows to access to the DX12's Copy engine interface.
class CopyingManager : public CommandListManager {
	friend Presenter;
	friend DeviceManager;
	friend ComputeManager;
	friend GPUScheduler;

	CopyingManager(DeviceManager* manager, ID3D12GraphicsCommandList5 *cmdList) :CommandListManager(manager, cmdList)
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

ComputeManager(DeviceManager* manager, ID3D12GraphicsCommandList5 *cmdList) : CopyingManager(manager, cmdList), setting(new Setter(this))
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

	GraphicsManager(DeviceManager* manager, ID3D12GraphicsCommandList5 *cmdList) :ComputeManager(manager, cmdList),
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

class DXRManager : public GraphicsManager {
	friend GPUScheduler;
	friend RTPipelineManager;
	//friend Dispatcher;
	friend GeometryCollection;
	friend InstanceCollection;
	friend IRTProgram;

	ID3D12RaytracingFallbackCommandList *fallbackCmdList;

	DXRManager(DeviceManager *manager, ID3D12GraphicsCommandList5 *cmdList) : GraphicsManager(manager, cmdList),
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
		DXRManager *manager;
	public:
		Setter(DXRManager *manager):manager(manager) {
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

		void UpdateShaderTable(gObj<Buffer> st, int index, ProgramHandle program) {
			byte shaderRecord[4096]; // max shader record size
			 //program.shaderHandle
		}

		// Commit all local bindings for this ray generation shader
		void RayGeneration(RayGenerationHandle program) {
			manager->activeRTProgram->UpdateRayGenLocals(manager, program);
		}

		void Miss(MissHandle program, int index) {
			manager->activeRTProgram->UpdateMissLocals(manager, program, index);
		}

		void HitGroup(HitGroupHandle group, int geometryIndex, 
			int rayContribution = 0, int multiplier = 1, int instanceContribution = 0)
		{
			int index = rayContribution + (geometryIndex * multiplier) + instanceContribution;
			manager->activeRTProgram->UpdateHitGroupLocals(manager, group, index);

		}

	} * const setting;

	class Dispatcher {
		DXRManager *manager;
	public:
		Dispatcher(DXRManager *manager) :manager(manager) {}
		void Rays(int width, int height, int depth = 1) {
			auto currentBindings = manager->currentPipeline1;
			auto currentProgram = manager->activeRTProgram;

			if (!currentBindings)
				return; // Exception?
			D3D12_DISPATCH_RAYS_DESC d;
			auto rtRayGenShaderTable = currentProgram->raygen_shaderTable;
			auto rtMissShaderTable = currentProgram->miss_shaderTable;
			auto rtHGShaderTable = currentProgram->group_shaderTable;

			auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
			{
				dispatchDesc->HitGroupTable.StartAddress = rtHGShaderTable->resource->GetGPUVirtualAddress();
				dispatchDesc->HitGroupTable.SizeInBytes = rtHGShaderTable->resource->desc.Width;
				dispatchDesc->HitGroupTable.StrideInBytes = rtHGShaderTable->Stride;

				dispatchDesc->MissShaderTable.StartAddress = rtMissShaderTable->resource->GetGPUVirtualAddress();
				dispatchDesc->MissShaderTable.SizeInBytes = rtMissShaderTable->resource->desc.Width;
				dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
				
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
		DXRManager * manager;
		Creating(DXRManager *manager) :manager(manager) {}
	public:
		gObj<TriangleGeometryCollection> TriangleGeometries() {
			return new TriangleGeometryCollection(manager->manager, manager);
		}
		gObj<InstanceCollection> Instances() {
			return new InstanceCollection(manager->manager, manager);
		}
	} * const creating;
};

#pragma endregion

#pragma region Techniques and Presenter

// Represents a creation module to create resources
class Creating {
	friend DeviceManager;
	friend CommandListManager;
	DeviceManager *manager;

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
	ResourceWrapper* CreateResourceAndWrap(const D3D12_RESOURCE_DESC &desc, D3D12_RESOURCE_STATES state, CPU_ACCESS cpuAccess = CPU_ACCESS_NONE) {

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

		ID3D12Resource *resource;
		auto hr = manager->device->CreateCommittedResource(
			cpuAccess == CPU_ACCESS_NONE ? &defaultProp : cpuAccess == CPU_WRITE_GPU_READ ? &uploadProp : &downloadProp, D3D12_HEAP_FLAG_NONE, &finalDesc, state, nullptr,
			IID_PPV_ARGS(&resource));

		if (FAILED(hr))
		{
			auto _hr = manager->device->GetDeviceRemovedReason();
			throw AfterShow(CA4G_Errors_RunOutOfMemory, TEXT(""), _hr);
		}
		return new ResourceWrapper(manager, desc, resource, state);
	}

	Creating(DeviceManager *manager) :manager(manager) {
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
		d.MipLevels = min (mips, MaxMipsFor(min(width, height)));
		d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		d.SampleDesc.Count = 1;
		d.SampleDesc.Quality = 0;
		d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d.Format = Formats<T>::Value;
		return new Texture2D (CreateResourceAndWrap(d, state));
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
		return new Texture2D(CreateResourceAndWrap(d, D3D12_RESOURCE_STATE_COMMON));
	}

	// Creates a buffer for constants using specifc struct type for size.
	// This buffer is created with CPU_WRITE_GPU_READ accessibility in order to allow
	// an efficient per frame one CPU write and one GPU read optimization. If your constant buffer
	// will be uploaded less frequently you can create a generic buffer instead.
	template<typename T>
	gObj<Buffer> ConstantBuffer() {
		return GenericBuffer<T>(D3D12_RESOURCE_STATE_GENERIC_READ, 1, CPU_WRITE_GPU_READ);
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
};

// Represents the loading module of a device manager.
// This class is used to load other techniques and pipeline objects.
class Loading {
	friend DeviceManager;

	DeviceManager *manager;
	Loading(DeviceManager* manager) :manager(manager) {}
public:
	// Loads a technique can be used as child technique.
	// Nevertheless, this engine proposes multiple inheritance of techniques instead for reusing logics.
	// Receives the variable to store the loaded technique and the list of arguments for creating the technique object.
	template<typename T, typename... A>
	void Subprocess(gObj<T> &technique, A... args)
	{
		technique = new T(args...);
		technique->BackBufferWidth = manager->BackBuffer->Width;
		technique->BackBufferHeight = manager->BackBuffer->Height;

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

// Represents a technique. This class is intended to be speciallized by users.
// implementing methods for Startup (initialization) and Frame (per-frame) logics.
// Use getManager() to access to the underlaying DX12 device.
class Technique {

	friend Presenter;
	bool isInitialized = false;
	void __OnInitialization(DeviceManager *manager) {
		this->manager = manager;
		this->loading = manager->loading;
		this->creating = manager->creating;
		Startup();
		isInitialized = true;
	}

	void __OnFrame() {
		Frame();
	}

	DeviceManager *manager;
protected:
	// Gets the backbuffer width
	int BackBufferWidth;

	// Gets the backbuffer height
	int BackBufferHeight;

	// Loading module to load other techniques and pipelines
	Loading *loading;
	
	// Creating module to create resources.
	Creating *creating;

	// Gets access to DX12 device manager.
	inline DeviceManager* getManager() {
		return manager;
	}

	// This method will be called only once when application starts
	virtual void Startup() {
	}

	// This method will be called every frame
	virtual void Frame() {
	}

public:
	virtual ~Technique(){}
};

// Represents a presenter object. This is a factory class that allows to create a 
// DeviceManager object, render targets for triplebuffering, Swapchain for presenting.
// Allows loading a technique and presenting it.
class Presenter {

	// Enable experimental features required for compute-based raytracing fallback.
	// This will set active D3D12 devices to DEVICE_REMOVED state.
	// Returns bool whether the call succeeded and the device supports the feature.
	inline bool EnableComputeRaytracingFallback(IDXGIAdapter1* adapter)
	{
		ID3D12Device* testDevice;
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };

		return SUCCEEDED(D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr))
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
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
	{
		IDXGIAdapter1* adapter;
		*ppAdapter = nullptr;

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

		*ppAdapter = adapter;
	}

	gObj<Texture2D>* renderTargetViews;
	DeviceManager* manager;
	IDXGISwapChain3* swapChain;

	unsigned int CurrentBuffer;

	void __PrepareToPresent(GraphicsManager* cmds) {
		renderTargetViews[CurrentBuffer]->ChangeStateTo(cmds->cmdList, D3D12_RESOURCE_STATE_PRESENT);
	}

	void __PrepareToRenderTarget(GraphicsManager* cmds) {
		renderTargetViews[CurrentBuffer]->ChangeStateTo(cmds->cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

public:

	ID3D12Debug *debugController;

	ID3D12Device* getInnerD3D12Device() {
		return manager->device;
	}

	ID3D12DescriptorHeap* getInnerDescriptorsHeap() {
		return manager->descriptors->gui_csu->heap;
	}

	// Creates a presenter object that creates de DX device attached to a specific window (hWnd).
	Presenter(HWND hWnd, bool fullScreen = false, int buffers = 2, bool useFrameBuffering = false, bool useWarpDevice = false) {
		UINT dxgiFactoryFlags = 0;
		ID3D12Device5 *device;

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

		IDXGIFactory4* factory;
		IDXGIInfoQueue* dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
		{
			CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
		else
			CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));


		if (useWarpDevice)
		{
			IDXGIAdapter *warpAdapter;
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
			IDXGIAdapter1 *hardwareAdapter;
			GetHardwareAdapter(factory, &hardwareAdapter);

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
				ID3D12Resource *rtResource;
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

	// Loads a technique.
	template<typename T, typename... A>
	void Load(gObj<T> &technique, A... args)
	{
		technique = new T(args...);
		technique->BackBufferWidth = manager->BackBuffer->Width;
		technique->BackBufferHeight = manager->BackBuffer->Height;
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
		
		manager->Perform(process(__PrepareToRenderTarget));

		technique->__OnFrame();
		
#pragma region GUI

		manager->Flush(ENGINE_MASK_ALL);

		// Use temporarly CmdList of manager[0] for GUI drawing

		manager->Scheduler->engines[0].threadInfos[0].Activate(manager->Scheduler->engines[0].frames[CurrentBuffer].allocatorSet[0]);

		auto gui_cmd = manager->Scheduler->engines[0].threadInfos[0].cmdList;

		manager->Perform(process(__PrepareToRenderTarget));

		D3D12_CPU_DESCRIPTOR_HANDLE rthandle = manager->BackBuffer->getRTVHandle();
		gui_cmd->OMSetRenderTargets(1, &rthandle, false, nullptr);
		gui_cmd->SetDescriptorHeaps(1, &this->manager->descriptors->gui_csu->heap);

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gui_cmd);

#pragma endregion

		manager->Perform(process(__PrepareToPresent));
		
		manager->Flush(ENGINE_MASK_ALL);

		auto hr = swapChain->Present(0, 0);
		
		manager->Scheduler->FinishFrame();

		if (hr != S_OK) {
			HRESULT r = manager->device->GetDeviceRemovedReason();
			throw AfterShow(CA4G_Errors_Any, TEXT(""), r);
		}
	}


	// Flush all pending work, and release objects.
	void Close() {
		manager->WaitFor(manager->SendSignal(manager->Flush(ENGINE_MASK_ALL)));

		delete manager;
	}
};

#pragma endregion

/* GENERIC IMPLEMENTATIONS */
#include "ca4g_impl.h"
