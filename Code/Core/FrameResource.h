#pragma once
#include "Core/UploadBuffer.h"

struct ObjectConstants {
    XMFLOAT4X4 World;
    XMFLOAT4X4 WorldViewProj;
};

struct LightBuffer {
    XMFLOAT3 LightDir = {};
    float padding0 = {};
    XMFLOAT3 LightColor = {};
};

struct CameraBuffer {
    XMFLOAT4X4 View;
    XMFLOAT4X4 Proj;
    XMFLOAT4X4 ProjectionTex;
    XMFLOAT4X4 ProjectionInv;
    XMFLOAT3 CameraPos;
    float ScreenWidth;
    XMFLOAT3 CameraUpAxis;
    float ScreenHeight;
    float ElapsedTime;
};

struct ShadowConstants
{
    XMFLOAT4X4 LightView;
    XMFLOAT4X4 LightProj;
    XMFLOAT4X4 ShadowTransform;
    XMFLOAT3 LightDir;
    float NearZ;
    XMFLOAT3 LightPosW;
    float FarZ;
};

constexpr UINT MAX_OBJECT_COUNT = 10000;
constexpr UINT MAX_LIGHT_COUNT = 1;
constexpr UINT MAX_CAMERA_COUNT = 1;
constexpr UINT MAX_SHADOW_COUNT = 1;

struct FrameResource final {
    FrameResource(ID3D12Device* device);
    FrameResource(const FrameResource&) = delete;
    FrameResource& operator=(const FrameResource&) = delete;
    ~FrameResource();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    std::unique_ptr<UploadBuffer> ObjectCB = nullptr;
    std::unique_ptr<UploadBuffer> LightCB = nullptr;
    std::unique_ptr<UploadBuffer> CameraCB = nullptr;
    std::unique_ptr<UploadBuffer> ShadowCB = nullptr;

    UINT64 Fence = 0;
};

