#pragma once

#include "ca4g.h"
#include <stdint.h>

#pragma region Command List Manager

void CommandListManager::__All(gObj<ResourceWrapper> & const dst, gObj<ResourceWrapper> & const src) {

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

void IRTProgram::BindOnGPU(DXRManager *manager, gObj<BindingsHandle> globals) {
	ID3D12GraphicsCommandList* cmdList = manager->cmdList;
	ID3D12RaytracingFallbackCommandList* fallbackCmdList = manager->fallbackCmdList;
	// Foreach bound slot
	for (int i = 0; i < globals->csuBindings.size(); i++)
	{
		auto binding = globals->csuBindings[i];

		switch (binding.Root_Parameter.ParameterType)
		{
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			cmdList->SetComputeRoot32BitConstants(i, binding.Root_Parameter.Constants.Num32BitValues,
				binding.ConstantData.ptrToConstant, 0);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
		{
#pragma region DESCRIPTOR TABLE
			// Gets the range length (if bound an array) or 1 if single.
			int count = binding.DescriptorData.ptrToCount == nullptr ? 1 : *binding.DescriptorData.ptrToCount;

			// Gets the bound resource if single
			gObj<ResourceView> resource = binding.DescriptorData.ptrToCount == nullptr ? *((gObj<ResourceView>*)binding.DescriptorData.ptrToResourceViewArray) : nullptr;

			// Gets the bound resources if array or treat the resource as a single array case
			gObj<ResourceView>* resourceArray = binding.DescriptorData.ptrToCount == nullptr ? &resource
				: *((gObj<ResourceView>**)binding.DescriptorData.ptrToResourceViewArray);

			// foreach resource in bound array (or single resource treated as array)
			for (int j = 0; j < count; j++)
			{
				// reference to the j-th resource (or bind null if array is null)
				gObj<ResourceView> resource = resourceArray == nullptr ? nullptr : *(resourceArray + j);

				if (!resource)
					// Grant a resource view to create null descriptor if missing resource.
					resource = ResourceView::getNullView(this->manager, binding.DescriptorData.Dimension);

				// Gets the cpu handle at not visible descriptor heap for the resource
				D3D12_CPU_DESCRIPTOR_HANDLE handle;
				resource->getCPUHandleFor(binding.Root_Parameter.DescriptorTable.pDescriptorRanges[0].RangeType, handle);

				// Adds the handle of the created descriptor into the src list.
				manager->srcDescriptors.add(handle);
			}
			// add the descriptors range length
			manager->dstDescriptorRangeLengths.add(count);
			int startIndex = this->manager->descriptors->gpu_csu->Malloc(count);
			manager->dstDescriptors.add(this->manager->descriptors->gpu_csu->getCPUVersion(startIndex));
			cmdList->SetComputeRootDescriptorTable(i, this->manager->descriptors->gpu_csu->getGPUVersion(startIndex));
			break; // DESCRIPTOR TABLE
#pragma endregion
		}
		case D3D12_ROOT_PARAMETER_TYPE_CBV:
		{
#pragma region DESCRIPTOR CBV
			// Gets the range length (if bound an array) or 1 if single.
			gObj<ResourceView> resource = *((gObj<ResourceView>*)binding.DescriptorData.ptrToResourceViewArray);
			cmdList->SetComputeRootConstantBufferView(i, resource->resource->GetGPUVirtualAddress());
			break; // DESCRIPTOR CBV
#pragma endregion
		}
		case D3D12_ROOT_PARAMETER_TYPE_SRV: // this parameter is used only for top level data structures
		{
#pragma region DESCRIPTOR SRV
			// Gets the range length (if bound an array) or 1 if single.
			gObj<SceneOnGPU> scene = *((gObj<SceneOnGPU>*)binding.SceneData.ptrToScene);
			
			if (manager->manager->fallbackDevice != nullptr)
			{ // Used Fallback device
				 fallbackCmdList->SetTopLevelAccelerationStructure(i, scene->topLevelAccFallbackPtr);
			}
			else
				cmdList->SetComputeRootShaderResourceView(i, scene->topLevelAccDS->resource->GetGPUVirtualAddress());
			break; // DESCRIPTOR CBV
#pragma endregion
		}
		}
	}
}

void IRTProgram::BindLocalsOnShaderTable(gObj<BindingsHandle> locals, byte* shaderRecordData) {
	for (int i = 0; i < locals->csuBindings.size(); i++)
	{
		auto binding = locals->csuBindings[i];

		switch (binding.Root_Parameter.ParameterType)
		{
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			memcpy(shaderRecordData, binding.ConstantData.ptrToConstant, binding.Root_Parameter.Constants.Num32BitValues * 4);
			shaderRecordData += binding.Root_Parameter.Constants.Num32BitValues * 4;
			break;
		default:
			shaderRecordData += 4 * 4;
		}
	}
}

// Create a wrapped pointer for the Fallback Layer path.
WRAPPED_GPU_POINTER DeviceManager::CreateFallbackWrappedPointer(gObj<Buffer> resource, UINT bufferNumElements)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC rawBufferUavDesc = {};
	rawBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	rawBufferUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	rawBufferUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	rawBufferUavDesc.Buffer.NumElements = bufferNumElements;

	D3D12_CPU_DESCRIPTOR_HANDLE bottomLevelDescriptor;

	// Only compute fallback requires a valid descriptor index when creating a wrapped pointer.
	UINT descriptorHeapIndex = 0;
	if (!fallbackDevice->UsingRaytracingDriver())
	{
		descriptorHeapIndex = descriptors->gpu_csu->MallocPersistent();
		bottomLevelDescriptor = descriptors->gpu_csu->getCPUVersion(descriptorHeapIndex);
		device->CreateUnorderedAccessView(resource->resource->internalResource, nullptr, &rawBufferUavDesc, bottomLevelDescriptor);
	}
	return fallbackDevice->GetWrappedPointerSimple(descriptorHeapIndex, resource->resource->internalResource->GetGPUVirtualAddress());
}
/*
void SceneBuilder::Build(DXRManager* manager) {
	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = instances.size();
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};

	if (manager->manager->fallbackDevice != nullptr)
	{
		manager->manager->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	}
	else // DirectX Raytracing
	{
		manager->manager->device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = buildFlags;
	bottomLevelInputs.NumDescs = geometries.size();
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometries.first();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	if (manager->manager->fallbackDevice != nullptr)
	{
		manager->manager->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	}
	else // DirectX Raytracing
	{
		manager->manager->device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	}

	D3D12_RESOURCE_STATES initialResourceState;
	if (manager->manager->fallbackDevice != nullptr)
	{
		initialResourceState = manager->manager->fallbackDevice->GetAccelerationStructureResourceState();
	}
	else // DirectX Raytracing
	{
		initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}
	lastBakedTopLevel = manager->manager->creating->GenericBuffer<byte>(initialResourceState, topLevelPrebuildInfo.ResultDataMaxSizeInBytes,
		CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	lastBakedBottomLevel = manager->manager->creating->GenericBuffer<byte>(initialResourceState, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes,
		CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Create instance buffers for top level ds bound to bottom level positions

	if (manager->manager->fallbackDevice != nullptr) {
		// Create an instance desc for the bottom-level acceleration structure.
	}
	else {

	}

	gObj<Buffer> instancesBuffer = manager->manager->creating->GenericBuffer<byte>();

	// build both acc ds

}
*/

gObj<GeometriesOnGPU> GeometryCollection::Creating::BakedGeometry() {
	// creates the bottom level acc ds and emulated gpu pointer if necessary
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = buildFlags;
	inputs.NumDescs = manager->geometries.size();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.pGeometryDescs = &manager->geometries.first();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	if (manager->manager->fallbackDevice != nullptr)
	{
		manager->manager->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
	}
	else // DirectX Raytracing
	{
		manager->manager->device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
	}

	D3D12_RESOURCE_STATES initialResourceState;
	if (manager->manager->fallbackDevice != nullptr)
	{
		initialResourceState = manager->manager->fallbackDevice->GetAccelerationStructureResourceState();
	}
	else // DirectX Raytracing
	{
		initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}

	gObj<Buffer> scratchBuffer = manager->manager->creating->GenericBuffer<byte>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		prebuildInfo.ScratchDataSizeInBytes, CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	gObj<Buffer> buffer = manager->manager->creating->GenericBuffer<byte>(initialResourceState,
		prebuildInfo.ResultDataMaxSizeInBytes, CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = inputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->resource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = buffer->resource->GetGPUVirtualAddress();
	}

	if (manager->manager->fallbackDevice != nullptr)
	{
		ID3D12DescriptorHeap *pDescriptorHeaps[] = { 
			manager->manager->descriptors->gpu_csu->getInnerHeap(), 
			manager->manager->descriptors->gpu_smp->getInnerHeap() };
		manager->cmdList->fallbackCmdList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
		manager->cmdList->fallbackCmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
	}
	else
		manager->cmdList->cmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

	buffer->ChangeStateTo(manager->cmdList->cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	gObj<GeometriesOnGPU> result = new GeometriesOnGPU();
	result->bottomLevelAccDS = buffer;
	result->scratchBottomLevelAccDS = scratchBuffer;

	if (manager->manager->fallbackDevice != nullptr) {
		// store an emulated gpu pointer via UAV
		UINT numBufferElements = static_cast<UINT>(prebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
		result->emulatedPtr = manager->manager->CreateFallbackWrappedPointer(buffer, numBufferElements);
	}

	return result;
}

gObj<SceneOnGPU> InstanceCollection::Creating::BakedScene() {
	// Bake scene using instance buffer and generate the top level DS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = buildFlags;
	
	if (manager->manager->fallbackDevice != nullptr)
		inputs.NumDescs = manager->fallbackInstances.size();
	else
		inputs.NumDescs = manager->instances.size();
	
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	if (manager->manager->fallbackDevice != nullptr)
		manager->manager->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
	else // DirectX Raytracing
		manager->manager->device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	gObj<Buffer> scratchBuffer = manager->manager->creating->GenericBuffer<byte>(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		prebuildInfo.ScratchDataSizeInBytes, CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_RESOURCE_STATES initialResourceState;
	if (manager->manager->fallbackDevice != nullptr)
	{
		initialResourceState = manager->manager->fallbackDevice->GetAccelerationStructureResourceState();
	}
	else // DirectX Raytracing
	{
		initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}

	gObj<Buffer> buffer = manager->manager->creating->GenericBuffer<byte>(initialResourceState,
		prebuildInfo.ResultDataMaxSizeInBytes, CPU_ACCESS_NONE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	gObj<Buffer> instanceBuffer;
	
	if (manager->manager->fallbackDevice != nullptr) {
		instanceBuffer = manager->manager->creating->GenericBuffer<byte>(D3D12_RESOURCE_STATE_GENERIC_READ,
			sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC)*manager->fallbackInstances.size(), CPU_WRITE_GPU_READ, D3D12_RESOURCE_FLAG_NONE);
		instanceBuffer->resource->UpdateMappedData(0, (void*)&manager->fallbackInstances.first(), sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC)*manager->fallbackInstances.size());
	}
	else {
		instanceBuffer = manager->manager->creating->GenericBuffer<byte>(D3D12_RESOURCE_STATE_GENERIC_READ,
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC)*manager->instances.size(), CPU_WRITE_GPU_READ, D3D12_RESOURCE_FLAG_NONE);
		instanceBuffer->resource->UpdateMappedData(0, (void*)&manager->instances.first(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC)*manager->instances.size());
	}

	// Build acc structure
	
	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		inputs.InstanceDescs = instanceBuffer->resource->internalResource->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = inputs;
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->resource->GetGPUVirtualAddress();
		topLevelBuildDesc.DestAccelerationStructureData = buffer->resource->GetGPUVirtualAddress();
	}

	if (manager->manager->fallbackDevice != nullptr)
	{
		ID3D12DescriptorHeap *pDescriptorHeaps[] = {
			manager->manager->descriptors->gpu_csu->getInnerHeap(),
			manager->manager->descriptors->gpu_smp->getInnerHeap() };
		manager->cmdList->fallbackCmdList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
		manager->cmdList->fallbackCmdList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	}
	else
		manager->cmdList->cmdList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	// Valid for Top level ds as well?
	buffer->ChangeStateTo(manager->cmdList->cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	gObj<SceneOnGPU> result = new SceneOnGPU();

	result->scratchBuffer = scratchBuffer;
	result->topLevelAccDS = buffer;
	result->instancesBuffer = instanceBuffer;
	result->usedGeometries = manager->usedGeometries;
	// Create a wrapped pointer to the acceleration structure.
	if (manager->manager->fallbackDevice != nullptr)
	{
		UINT numBufferElements = static_cast<UINT>(prebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
		result->topLevelAccFallbackPtr = manager->manager->CreateFallbackWrappedPointer(buffer, numBufferElements);
	}

	return result;
}

//void DXRStateBindings::BindOnGPU(DXRManager *manager, const list<SlotBinding> &list, int startRootParameter) {
//	ID3D12GraphicsCommandList* cmdList = manager->cmdList;
//
//	// Foreach bound slot
//	for (int i = 0; i < list.size(); i++)
//	{
//		auto binding = list[i];
//
//		// Gets the range length (if bound an array) or 1 if single.
//		int count = binding.ptrToCount == nullptr ? 1 : *binding.ptrToCount;
//
//		// Gets the bound resource if single
//		gObj<ResourceView> resource = binding.ptrToCount == nullptr ? *((gObj<ResourceView>*)binding.ptrToResource) : nullptr;
//
//		// Gets the bound resources if array or treat the resource as a single array case
//		gObj<ResourceView>* resourceArray = binding.ptrToCount == nullptr ? &resource
//			: *((gObj<ResourceView>**)binding.ptrToResource);
//
//		// foreach resource in bound array (or single resource treated as array)
//		for (int j = 0; j < count; j++)
//		{
//			// reference to the j-th resource (or bind null if array is null)
//			gObj<ResourceView> resource = resourceArray == nullptr ? nullptr : *(resourceArray + j);
//
//			if (!resource)
//				// Grant a resource view to create null descriptor if missing resource.
//				resource = ResourceView::getNullView(this->manager, binding.Dimension);
//
//			// Gets the cpu handle at not visible descriptor heap for the resource
//			D3D12_CPU_DESCRIPTOR_HANDLE handle;
//			resource->getCPUHandleFor(binding.type, handle);
//
//			// Adds the handle of the created descriptor into the src list.
//			manager->srcDescriptors.add(handle);
//		}
//		// add the descriptors range length
//		manager->dstDescriptorRangeLengths.add(count);
//		int startIndex = this->manager->descriptors->gpu_csu->Malloc(count);
//		manager->dstDescriptors.add(this->manager->descriptors->gpu_csu->getCPUVersion(startIndex));
//		cmdList->SetComputeRootDescriptorTable(startRootParameter + i, this->manager->descriptors->gpu_csu->getGPUVersion(startIndex));
//	}
//}

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

void RTPipelineManager::__OnSet(DXRManager * manager)
{
	ID3D12DescriptorHeap* heaps[] = { this->manager->descriptors->gpu_csu->getInnerHeap(), this->manager->descriptors->gpu_smp->getInnerHeap() };

	if (manager->fallbackCmdList)
		manager->fallbackCmdList->SetDescriptorHeaps(2, heaps);
	else
		manager->cmdList->SetDescriptorHeaps(2, heaps);
}

void IRTProgram::UpdateRayGenLocals(DXRManager* cmdList, RayGenerationHandle shader) {
	// Get shader identifier
	byte* shaderID;
	int shaderIDSize;
	if (manager->fallbackDevice != nullptr)
	{
		shaderIDSize = manager->fallbackDevice->GetShaderIdentifierSize();
		shaderID = (byte*)cmdList->currentPipeline1->fbso->GetShaderIdentifier(shader.shaderHandle);
	}
	else // DirectX Raytracing
	{
		shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		shaderID = (byte*)((ID3D12StateObjectPropertiesPrototype*)cmdList->currentPipeline1->so)->GetShaderIdentifier(shader.shaderHandle);
	}

	byte* shaderRecordStart = raygen_shaderTable->GetShaderRecordStartAddress(0);
	memcpy(shaderRecordStart, shaderID, shaderIDSize);
	if (!raygen_locals.isNull())
		BindLocalsOnShaderTable(raygen_locals, shaderRecordStart + shaderIDSize);
}

void IRTProgram::UpdateMissLocals(DXRManager* cmdList, MissHandle shader, int index)
{
	// Get shader identifier
	byte* shaderID;
	int shaderIDSize;
	int shaderRecordSize;
	if (manager->fallbackDevice != nullptr)
	{
		shaderIDSize = manager->fallbackDevice->GetShaderIdentifierSize();
		shaderID = (byte*)cmdList->currentPipeline1->fbso->GetShaderIdentifier(shader.shaderHandle);
	}
	else // DirectX Raytracing
	{
		shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		shaderID = (byte*)((ID3D12StateObjectPropertiesPrototype*)cmdList->currentPipeline1->so)->GetShaderIdentifier(shader.shaderHandle);
	}
	shaderRecordSize = shaderIDSize + ( miss_locals.isNull() ? 0 : miss_locals->rootSize );

	byte* shaderRecordStart = miss_shaderTable->GetShaderRecordStartAddress(0);
	memcpy(shaderRecordStart, shaderID, shaderIDSize);
	if (!miss_locals.isNull())
		BindLocalsOnShaderTable(miss_locals, shaderRecordStart + shaderIDSize);
}

void IRTProgram::UpdateHitGroupLocals(DXRManager* cmdList, HitGroupHandle shader, int index) {
	// Get shader identifier
	byte* shaderID;
	int shaderIDSize;
	int shaderRecordSize;
	if (manager->fallbackDevice != nullptr)
	{
		shaderIDSize = manager->fallbackDevice->GetShaderIdentifierSize();
		shaderID = (byte*)cmdList->currentPipeline1->fbso->GetShaderIdentifier(shader.shaderHandle);
	}
	else // DirectX Raytracing
	{
		shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		shaderID = (byte*)((ID3D12StateObjectPropertiesPrototype*)cmdList->currentPipeline1->so)->GetShaderIdentifier(shader.shaderHandle);
	}
	shaderRecordSize = shaderIDSize + (hitGroup_locals.isNull() ? 0 : hitGroup_locals->rootSize);

	byte* shaderRecordStart = group_shaderTable->GetShaderRecordStartAddress(0);
	memcpy(shaderRecordStart, shaderID, shaderIDSize);
	if (!hitGroup_locals.isNull())
		BindLocalsOnShaderTable(hitGroup_locals, shaderRecordStart + shaderIDSize);
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
	switch (dimension)
	{
	case D3D12_RESOURCE_DIMENSION_BUFFER:
		return manager->__NullBuffer ? manager->__NullBuffer : manager->__NullBuffer = new Buffer(manager);
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		return manager->__NullTexture2D ? manager->__NullTexture2D : manager->__NullTexture2D = new Texture2D(manager);
	}
	throw "not supported yet";
}

#pragma endregion

#pragma region DXRStateBindings
//void DXRStateBindings::__OnInitialization(DeviceManager * manager)
//{
//	this->manager = manager;
//	Setup();
//	__CurrentLoadingCSU = &__GlobalsCSU;
//	__CurrentLoadingSamplers = &__GlobalsSamplers;
//	Globals();
//	__CurrentLoadingCSU = &__LocalsCSU;
//	__CurrentLoadingSamplers = &__LocalsSamplers;
//	Locals();
//
//	int sizeShaderIdentifier = manager->fallbackDevice != nullptr ? manager->fallbackDevice->GetShaderIdentifierSize() :
//		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
//
//	raygenShaderTable = this->manager->creating->ShaderTable(D3D12_RESOURCE_STATE_GENERIC_READ, sizeShaderIdentifier);
//	missShaderTable = this->manager->creating->ShaderTable(D3D12_RESOURCE_STATE_GENERIC_READ, sizeShaderIdentifier);
//	hitgroupShaderTable = this->manager->creating->ShaderTable(D3D12_RESOURCE_STATE_GENERIC_READ, sizeShaderIdentifier);
//}

#pragma endregion

#pragma region Device Manager

DeviceManager::DeviceManager(ID3D12Device5 *device, int buffers, bool useFrameBuffering, bool isWarpDevice)
	:
	device(device),
	counting(new CountEvent()),
	descriptors(new DescriptorsManager(device)),
	creating(new Creating(this)),
	loading(new Loading(this))
{
	auto hr = D3D12CreateRaytracingFallbackDevice(device, CreateRaytracingFallbackDeviceFlags::ForceComputeFallback, 0, IID_PPV_ARGS(&fallbackDevice));

	if (FAILED(hr)) {
		throw AfterShow(CA4G_Errors_Unsupported_Fallback, nullptr, hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options;
	if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof(options)))
		&& options.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
	{
		fallbackDevice = nullptr; // device supports DXR! no necessary fallback device
	}

	Scheduler = new GPUScheduler(this, useFrameBuffering, CA4G_MAX_NUMBER_OF_WORKERS, buffers);
}

DeviceManager::~DeviceManager() {
	if (!__NullBuffer.isNull())
		__NullBuffer = nullptr;
	if (!__NullTexture2D.isNull())
		__NullTexture2D = nullptr;

	delete creating;
	delete counting;
	delete loading;
	delete Scheduler;
	if (fallbackDevice != nullptr)
		delete fallbackDevice;
	delete descriptors;
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
