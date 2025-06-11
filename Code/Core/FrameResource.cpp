#include "pch.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device)
{
	HR(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	ObjectCB = std::make_unique<UploadBuffer>(device, MAX_OBJECT_COUNT, true, sizeof(ObjectConstants));
	LightCB = std::make_unique<UploadBuffer>(device, MAX_LIGHT_COUNT, true, sizeof(LightBuffer));
	CameraCB = std::make_unique<UploadBuffer>(device, MAX_CAMERA_COUNT, true, sizeof(CameraBuffer));
	ShadowCB = std::make_unique<UploadBuffer>(device, MAX_SHADOW_COUNT, true, sizeof(ShadowConstants));
}

FrameResource::~FrameResource()
{

}