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
		mParticles.emplace_back( Particle(XMFLOAT3(0, 5, 0), XMFLOAT2(0.3f,0.3f)));
		mParticleData.emplace_back(ParticleData(RANDOM::InsideUnitSphere(), 15.0f, 1.0f));
	}
		 
	mVertexBufferView.BufferLocation = mParticleBuffer->Resource()->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = mCapacity * sizeof(Particle);
	mVertexBufferView.StrideInBytes = sizeof(Particle);
}

void ParticleGenerator::Update(const float deltaTime)
{
	int particleIndex = 0;
	for (int i = 0; i < mCapacity; ++i)
	{
		//TODO: Particle Movement Update.
		XMVECTOR vPosition = XMLoadFloat3(&mParticles[i].Position);
		vPosition += mParticleData[i].Velocity * mParticleData[i].Speed * deltaTime;
		XMStoreFloat3(&mParticles[i].Position, vPosition);

		mParticleData[i].LifeTime -= deltaTime;

		//Alive인 Particle만 담아야함.
		if (mParticleData[i].IsActive())
		{
			mParticleBuffer->CopyData(particleIndex++, mParticles[i]);
		}
		else
		{
			mParticleData[i].LifeTime = 1.0f;
			mParticleData[i].Velocity = RANDOM::InsideUnitSphere();
			mParticles[i].Position = XMFLOAT3(0, 5.0f, 0);
		}
	}

	mAliveCount = particleIndex;
}

void ParticleGenerator::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	pCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	pCommandList->DrawInstanced(mAliveCount, 1, 0, 0); // aliveCount는 살아있는 파티클 수
}
