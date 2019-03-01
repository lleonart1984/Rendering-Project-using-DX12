#pragma once

#include "ca4g.h"
#include <stdint.h>

#pragma region Command List Manager

void CommandListManager::__All(gObj<ResourceWrapper> dst, gObj<ResourceWrapper> src) {

	if (EquivalentDesc(dst->internalResource->GetDesc(), src->internalResource->GetDesc()))
	{ // have the same dimensions
		cmdList->CopyResource(dst->internalResource, src->internalResource);
	}
	else
	{
		if (dst->internalResource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{ // both buffers
			cmdList->CopyBufferRegion(
				dst->internalResource, 0, src->internalResource, 0, src->internalResource->GetDesc().Width);
		}
		else
		{
			// Buffer from upload to any resource on the gpu
			auto device = getInternalDevice();

			int numberOfSubresources = dst->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 :
				dst->desc.MipLevels * dst->desc.DepthOrArraySize;

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* Layouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[numberOfSubresources];
			UINT* NumRows = new UINT[numberOfSubresources];
			UINT64* BytesForRow = new UINT64[numberOfSubresources];
			UINT64 totalBytes;
			device->GetCopyableFootprints(&src->desc, 0, numberOfSubresources, 0, Layouts, NumRows, BytesForRow, &totalBytes);

			for (int i = 0; i < numberOfSubresources; i++) {

				D3D12_TEXTURE_COPY_LOCATION Dst{};
				Dst.pResource = dst->internalResource;
				Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				Dst.SubresourceIndex = i;

				D3D12_TEXTURE_COPY_LOCATION Src{};
				Src.pResource = src->internalResource;
				Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				Src.PlacedFootprint = Layouts[i];
				cmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
			}
			delete[] Layouts;
			delete[] NumRows;
			delete[] BytesForRow;
		}
	}
}

void CommandListManager::Loading::FromData(gObj<Texture2D> &texture, TextureData* data) {
	D3D12_RESOURCE_DESC d;
	ZeroMemory(&d, sizeof(D3D12_RESOURCE_DESC));
	d.Width = data->Width;
	d.Height = data->Height;
	d.Dimension = data->Dimension;
	d.DepthOrArraySize = data->ArraySlices;
	d.MipLevels = data->MipMaps;
	d.Flags = D3D12_RESOURCE_FLAG_NONE;
	d.SampleDesc.Count = 1;
	d.SampleDesc.Quality = 0;
	d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d.Format = data->Format;
	texture = new Texture2D(this->manager->manager->creating->CreateResourceAndWrap(d, D3D12_RESOURCE_STATE_COPY_DEST));
	texture->resource->CreateForUploading();
	texture->resource->uploadingResourceCache->UploadFullData(data->Data, data->DataSize, data->FlipY);
	manager->__All(texture->resource, texture->resource->uploadingResourceCache);
}

inline ID3D12Device* CommandListManager::getInternalDevice() { return manager->device; }

template<typename ...A>
void PipelineBindings<A...>::BindOnGPU(ComputeManager *manager, const list<SlotBinding> &list, int startRootParameter) {
	ID3D12GraphicsCommandList* cmdList = manager->cmdList;

	// Foreach bound slot
	for (int i = 0; i < list.size(); i++)
	{
		auto binding = list[i];
		
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
void PipelineBindings<A...>::__OnSet(ComputeManager *manager) {
	ID3D12GraphicsCommandList* cmdList = manager->cmdList;
	for (int i = 0; i < RenderTargetMax; i++)
		if (!(*RenderTargets[i]))
			RenderTargetDescriptors[i] = ResourceView::getNullView(this->manager, D3D12_RESOURCE_DIMENSION_TEXTURE2D)->getRTVHandle();
		else
			RenderTargetDescriptors[i] = (*RenderTargets[i])->getRTVHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE depthHandle;
	if (!(DepthBufferField == nullptr || !(*DepthBufferField)))
		depthHandle = (*DepthBufferField)->getDSVHandle();

	cmdList->OMSetRenderTargets(RenderTargetMax, RenderTargetDescriptors, FALSE, (DepthBufferField == nullptr || !(*DepthBufferField)) ? nullptr : &depthHandle);
	ID3D12DescriptorHeap* heaps[] = { this->manager->descriptors->gpu_csu->getInnerHeap(), this->manager->descriptors->gpu_smp->getInnerHeap() };
	cmdList->SetDescriptorHeaps(2, heaps);

	BindOnGPU(manager, __GlobalsCSU, 0);
}

#pragma endregion

#pragma region Descriptors

CPUDescriptorHeapManager::CPUDescriptorHeapManager(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity) {
	size = device->GetDescriptorHandleIncrementSize(type);
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NodeMask = 0;
	desc.NumDescriptors = capacity;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	startCPU = heap->GetCPUDescriptorHandleForHeapStart().ptr;
}

int CPUDescriptorHeapManager::AllocateNewHandle() {
	int index;
	mutex.Acquire();
	if (free != nullptr) {
		index = free->index;
		Node *toDelete = free;
		free = free->next;
		delete toDelete;
	}
	else
		index = allocated++;
	mutex.Release();
	return index;
}

GPUDescriptorHeapManager::GPUDescriptorHeapManager(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, int capacity) :capacity(capacity){
	size = device->GetDescriptorHandleIncrementSize(type);
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NodeMask = 0;
	desc.NumDescriptors = capacity;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	auto hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr))
		throw AfterShow(CA4G_Errors_RunOutOfMemory, TEXT("Creating descriptor heaps."));
	startCPU = heap->GetCPUDescriptorHandleForHeapStart().ptr;
	startGPU = heap->GetGPUDescriptorHandleForHeapStart().ptr;
	mallocOffset = 0;
}
#pragma endregion

#pragma region Resource View

gObj<ResourceView> ResourceView::getNullView(DeviceManager* manager, D3D12_RESOURCE_DIMENSION dimension)
{
	static gObj<Buffer> NullBuffer = nullptr;
	static gObj<Texture2D> NullTexture2D = nullptr;
	switch (dimension)
	{
	case D3D12_RESOURCE_DIMENSION_BUFFER:
		return NullBuffer ? NullBuffer : NullBuffer = new Buffer(manager);
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		return NullTexture2D ? NullTexture2D : NullTexture2D = new Texture2D(manager);
	}
	throw "not supported yet";
}

#pragma endregion

#pragma region Device Manager

DeviceManager::DeviceManager(ID3D12Device5 *device, int buffers, bool useFrameBuffering)
	:
	device(device),
	counting(new CountEvent()),
	descriptors(new DescriptorsManager(device)),
	creating(new Creating(this)),
	loading(new Loading(this))
{
	auto hr = D3D12CreateRaytracingFallbackDevice(device, 0, 0, IID_PPV_ARGS(&fallbackDevice));

	if (FAILED(hr)) {
		throw AfterShow(CA4G_Errors_Unsupported_Fallback, nullptr, hr);
	}

	Scheduler = new GPUScheduler(this, useFrameBuffering, CA4G_MAX_NUMBER_OF_WORKERS, buffers);
}

DeviceManager::~DeviceManager() {
	delete descriptors;
	delete creating;
	delete counting;
	delete loading;
	delete Scheduler;
}

#pragma endregion

#pragma region Engine implementation

GPUScheduler::GPUScheduler(DeviceManager* manager, bool useFrameBuffering, int max_threads, int buffers) 
	: 
	manager(manager),
	useFrameBuffer(useFrameBuffering)
{
	ID3D12Device5 *device = manager->device;

	// Creating the engines (engines are shared by all frames and threads...)
	for (int e = 0; e < CA4G_SUPPORTED_ENGINES; e++) // 0 - direct, 1 - bunddle, 2 - compute, 3 - copy.
	{
		D3D12_COMMAND_LIST_TYPE type = (D3D12_COMMAND_LIST_TYPE)e;
		queues[e] = new CommandQueueManager(device, type);

		engines[e].frames = new PerEngineInfo::PerFrameInfo[buffers];

		for (int f = 0; f < buffers; f++) {
			PerEngineInfo::PerFrameInfo &frame = engines[e].frames[f];
			frame.allocatorSet = new ID3D12CommandAllocator*[max_threads];
			frame.allocatorsUsed = new bool[max_threads];
			for (int t = 0; t < max_threads; t++)
				// creating gpu allocator for each engine, each frame, and each thread
				device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.allocatorSet[t]));
		}

		engines[e].queue = queues[e];
		engines[e].threadInfos = new PerEngineInfo::PerThreadInfo[max_threads];
		for (int t = 0; t < max_threads; t++) {
			PerEngineInfo::PerThreadInfo &thread = engines[e].threadInfos[t];

			device->CreateCommandList(0, type, engines[e].frames[0].allocatorSet[0], nullptr, IID_PPV_ARGS(&thread.cmdList));
			thread.cmdList->Close(); // start cmdList state closed.
			thread.isActive = false;

			switch (type) {
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				thread.manager = new DXRManager(manager, thread.cmdList);
				break;
			case D3D12_COMMAND_LIST_TYPE_BUNDLE:
				thread.manager = new GraphicsManager(manager, thread.cmdList);
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				thread.manager = new ComputeManager(manager, thread.cmdList);
				break;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				thread.manager = new CopyingManager(manager, thread.cmdList);
				break;
			}
		}
	}

	// Thread safe concurrent queue for enqueuing work to do asynchronously
	workToDo = new ProducerConsumerQueue<ICallableMember*>(max_threads);

	// Creating worker structs and threads
	workers = new GPUWorkerInfo[max_threads];
	threads = new HANDLE[max_threads];
	counting = new CountEvent();
	for (int i = 0; i < max_threads; i++) {
		workers[i] = { i, this };

		DWORD threadId;
		if (i > 0) // only create threads for workerIndex > 0. Worker 0 will execute on main thread
			threads[i] = CreateThread(nullptr, 0, __WORKER_TODO, &workers[i], 0, &threadId);
	}

	__activeCmdLists = new ID3D12GraphicsCommandList*[max_threads];

	perFrameFinishedSignal = new Signal[buffers];

	this->threadsCount = max_threads;
}

int GPUScheduler::Flush(int mask) {
	counting->Wait();

	int resultMask = 0;

	for (int e = 0; e < CA4G_SUPPORTED_ENGINES; e++)
		if (mask & (1 << e))
		{
			int activeCmdLists = 0;
			for (int t = 0; t < threadsCount; t++) {
				if (engines[e].threadInfos[t].isActive) // pending work here
				{
					engines[e].threadInfos[t].Close();
					__activeCmdLists[activeCmdLists++] = engines[e].threadInfos[t].cmdList;
				}
				auto manager = engines[e].threadInfos[t].manager;

				// Copy all collected descriptors from non-visible to visible DHs.
				if (manager->srcDescriptors.size() > 0)
				{
					this->manager->device->CopyDescriptors(
						manager->dstDescriptors.size(), &manager->dstDescriptors.first(), &manager->dstDescriptorRangeLengths.first(),
						manager->srcDescriptors.size(), &manager->srcDescriptors.first(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
					);
					// Clears the lists for next usage
					manager->srcDescriptors.reset();
					manager->dstDescriptors.reset();
					manager->dstDescriptorRangeLengths.reset();
				}
			}

			if (activeCmdLists > 0) // some cmdlist to execute
			{
				resultMask |= 1 << e;
				queues[e]->Commit(__activeCmdLists, activeCmdLists);
			}
		}

	return resultMask;
}

void GPUScheduler::PopulateCommandsBy(ICallableMember *process, int workerIndex) {
	int engineIndex = process->getType();

	auto cmdListManager = engines[engineIndex].threadInfos[workerIndex].manager;

	engines[engineIndex].threadInfos[workerIndex].Activate(engines[engineIndex].frames[currentFrameIndex].RequireAllocator(workerIndex));

	cmdListManager->Tag = process->TagID;
	process->Call(cmdListManager);

	if (workerIndex > 0)
		counting->Signal();
}

DWORD WINAPI GPUScheduler::__WORKER_TODO(LPVOID param) {
	GPUWorkerInfo* wi = (GPUWorkerInfo*)param;
	int index = wi->Index;
	GPUScheduler* scheduler = wi->Scheduler;

	while (!scheduler->isClosed()) {
		ICallableMember* nextProcess;
		if (!scheduler->workToDo->TryConsume(nextProcess))
			break;

		scheduler->PopulateCommandsBy(nextProcess, index);
		
		delete nextProcess;
	}
	return 0;
}

#pragma endregion
