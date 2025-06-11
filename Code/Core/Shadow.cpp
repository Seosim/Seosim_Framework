#include "pch.h"
#include "Shadow.h"

void Shadow::Release()
{
	RELEASE_COM(mShadow);
}

void Shadow::Initialize(ID3D12Device* pDevice, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap, ID3D12Resource* pResource)
{
	mShadow = pResource;

	mShadowBuffer.LightDir = { 0.577350020,  -0.577350020, 0.577350020 };

	mShadowCB = std::make_unique<UploadBuffer>(pDevice, 1, true, sizeof(ShadowConstants));

	mShadowShader = new Shader();
	auto command = Shader::DefaultCommand();
	command.SampleCount = 1;
	mShadowShader->Initialize(pDevice, pRootSignature, "Shadow", command);

	mShadowTexture = new Texture();
	mShadowTexture->CreateSrvWithResource(pDevice, pSrvHeap, L"ShadowMap", mShadow, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, false);

	mShadowViewport = { 0.0f, 0.0f, (float)SHADOW_MAP_SIZE, (float)SHADOW_MAP_SIZE, 0.0f, 1.0f };
	mShadowScissorRect = { 0, 0, (int)SHADOW_MAP_SIZE, (int)SHADOW_MAP_SIZE };
}

void Shadow::ChangeResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, ID3D12Resource* pResource)
{
	mShadow = pResource;

	if (mShadowTexture)
		mShadowTexture->ChangeResource(pDevice, pSrvHeap, L"ShadowMap", mShadow, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, false);
}

void Shadow::SetViewPortAndScissorRect(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->RSSetViewports(1, &mShadowViewport);
	pCommandList->RSSetScissorRects(1, &mShadowScissorRect);
}

void Shadow::SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	D3D12_GPU_DESCRIPTOR_HANDLE texHandle = pSrvHeap->GetGPUDescriptorHandleForHeapStart();
	texHandle.ptr += 32 * (mShadowTexture->GetID());
	pCommandList->SetGraphicsRootDescriptorTable((int)eRootParameter::SHADOW_TEXTURE, texHandle);
}

void Shadow::UpdateShadowTransform(ID3D12GraphicsCommandList* pCommandList, XMVECTOR cameraPos, XMVECTOR cameraForward)
{
	//float radius = 150.0f;
	float focusDistance = 100.0f;

	float halfFov = XMConvertToRadians(60.0f * 0.5f);
	float halfHeight = tanf(halfFov) * focusDistance;
	float halfWidth = halfHeight * (16.0f / 9);

	float radius = std::max(halfWidth, halfHeight) * 1.2f; // 여유 공간 포함

	// 카메라 위치 기준 타겟 설정
	XMVECTOR lightDir = XMLoadFloat3(&mShadowBuffer.LightDir);
	
	XMVECTOR targetPos = cameraPos + cameraForward * focusDistance;
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// 이 스냅된 위치로 다시 lightPos, lightView 생성
	XMVECTOR lightPos = targetPos - 2.0f * radius * lightDir;
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mShadowBuffer.LightPosW, lightPos);

	// 타겟 포지션을 기준으로 라이트 공간 중심 계산
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum 설정
	float l = sphereCenterLS.x - radius;
	float b = sphereCenterLS.y - radius;
	float n = sphereCenterLS.z - radius;
	float r = sphereCenterLS.x + radius;
	float t = sphereCenterLS.y + radius;
	float f = sphereCenterLS.z + radius;

	mShadowBuffer.NearZ = n;
	mShadowBuffer.FarZ = f;

	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// NDC → Texture space
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// 최종 ShadowTransform 계산
	XMMATRIX S = lightView * lightProj * T;

	XMStoreFloat4x4(&mShadowBuffer.LightView, XMMatrixTranspose(lightView));
	XMStoreFloat4x4(&mShadowBuffer.LightProj, XMMatrixTranspose(lightProj));
	XMStoreFloat4x4(&mShadowBuffer.ShadowTransform, XMMatrixTranspose(S));

	// 상수 버퍼 업데이트 및 바인딩
	mShadowCB->CopyData(0, mShadowBuffer);
	pCommandList->SetGraphicsRootConstantBufferView((int)eRootParameter::SHADOW, mShadowCB->Resource()->GetGPUVirtualAddress());
	mShadowShader->SetPipelineState(pCommandList);
}

void Shadow::Render()
{
}

ID3D12Resource* Shadow::GetResource()
{
	return mShadow;
}

XMFLOAT3 Shadow::GetShadowDirection() const
{
	return mShadowBuffer.LightDir;
}
