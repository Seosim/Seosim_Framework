#include "pch.h"
#include "ParticleGenerator.h"

void ParticleGenerator::Initialize(ID3D12Device* pDevice, const UINT particleCount)
{
	mCapacity = particleCount;
	mParticles.reserve(mCapacity);
	mParticleData.reserve(mCapacity);

	mParticleBuffer = std::make_unique<UploadBuffer>(pDevice, particleCount, false, sizeof(Particle));

	//HACK: 초기값 강제 삽입
	for (int i = 0; i < mCapacity; ++i)
	{
		mParticles.emplace_back( Particle(XMFLOAT3(0, 5, 0), XMFLOAT2(1,1)));
		mParticleData.emplace_back(ParticleData(5.0f, 1.0f));
	}
		 
	mVertexBufferView.BufferLocation = mParticleBuffer->Resource()->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = mCapacity * sizeof(Particle);
	mVertexBufferView.StrideInBytes = sizeof(Particle);
}

void ParticleGenerator::Update(const float deltaTime)
{
	static float timer = 0.0f;
	int particleIndex = 0;
	for (int i = 0; i < mCapacity; ++i)
	{
		//TODO: Particle Movement Update.
		mParticles[i].Position.y = std::sin(timer) * 5.0f;

		//Alive인 Particle만 담아야함.
		mParticleBuffer->CopyData(particleIndex++, mParticles[i]);
	}

	mAliveCount = particleIndex;
	timer += deltaTime;
}

void ParticleGenerator::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	pCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	pCommandList->DrawInstanced(mAliveCount, 1, 0, 0); // aliveCount는 살아있는 파티클 수
}
