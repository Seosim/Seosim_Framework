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

void Shadow::UpdateShadowTransform(ID3D12GraphicsCommandList* pCommandList)
{
	//float radius = 20.0277557f;
	float radius = 100.0f;

	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mShadowBuffer.LightDir);
	XMVECTOR targetPos = XMVectorSet(0, 0, 0, 1);
	XMVECTOR lightPos = (-2.0f * radius * lightDir);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mShadowBuffer.LightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - radius;
	float b = sphereCenterLS.y - radius;
	float n = sphereCenterLS.z - radius;
	float r = sphereCenterLS.x + radius;
	float t = sphereCenterLS.y + radius;
	float f = sphereCenterLS.z + radius;

	mShadowBuffer.NearZ = n;
	mShadowBuffer.FarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mShadowBuffer.LightView, XMMatrixTranspose(lightView));
	XMStoreFloat4x4(&mShadowBuffer.LightProj, XMMatrixTranspose(lightProj));
	XMStoreFloat4x4(&mShadowBuffer.ShadowTransform, XMMatrixTranspose(S));

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
