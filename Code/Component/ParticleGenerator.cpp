#include "pch.h"
#include "ParticleGenerator.h"

void ParticleGenerator::Initialize(ID3D12Device* pDevice, const UINT particleCount, eShape shape)
{
	mCapacity = particleCount;
	mShape = shape;
	mParticles.reserve(mCapacity);
	mParticleData.reserve(mCapacity);

	mParticleBuffer = std::make_unique<UploadBuffer>(pDevice, particleCount, false, sizeof(Particle));

	//HACK: 초기값 강제 삽입
	for (int i = 0; i < mCapacity; ++i)
	{
		XMFLOAT3 position = {};
		XMVECTOR direction = {};

		if (mShape == Sphere)
		{
			position = XMFLOAT3(0.0f, 5.0f, 0.0f);
			direction = RANDOM::InsideUnitSphere();
		}
		else if (mShape == Cone)
		{
			XMVECTOR forward = XMVectorSet(1, 0, 0, 0); // 기준 방향
			constexpr float coneAngle = XMConvertToRadians(45.0f); // 콘의 반각도
			constexpr float radius = 5.0f;
			direction = RANDOM::DirectionInCone(forward, coneAngle);
			XMVECTOR randomPos = direction * radius;
			randomPos += XMVectorSet(0, 5, 0, 0);
			XMStoreFloat3(&position, randomPos);
		}
		else if (mShape == Circle)
		{
			XMVECTOR axis = XMVectorSet(1, 0, 0, 0); // 기준 방향
			XMStoreFloat3(&position, RANDOM::CircleEdgePoint(axis, 5.0f, &direction) + XMVectorSet(0, 10, 0, 0));
		}
		else
		{
			ASSERT(false);
		}

		mParticles.emplace_back(Particle(position, 1.0f, XMFLOAT2(1.0f, 1.0f)));
		mParticleData.emplace_back(ParticleData(direction, 30.0f, 0.1f, 0.1f));
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
		mParticles[i].LifeFactor = 0.7f + mParticleData[i].LifeTime / mParticleData[i].MaxLifeTime * (1.0f - 0.7f);

		//Alive인 Particle만 담아야함.
		if (mParticleData[i].IsActive())
		{
			mParticleBuffer->CopyData(particleIndex++, mParticles[i]);
		}
		else
		{
			XMFLOAT3 position = {};
			XMVECTOR direction = {};

			if (mShape == Sphere)
			{
				position = XMFLOAT3(0.0f, 5.0f, 0.0f);
				direction = RANDOM::InsideUnitSphere();
			}
			else if (mShape == Cone)
			{
				XMVECTOR forward = XMVectorSet(1, 0, 0, 0); // 기준 방향
				constexpr float coneAngle = XMConvertToRadians(45.0f); // 콘의 반각도
				constexpr float radius = 5.0f;
				direction = RANDOM::DirectionInCone(forward, coneAngle);
				XMVECTOR randomPos = direction * radius;
				randomPos += XMVectorSet(0, 5, 0, 0);
				XMStoreFloat3(&position, randomPos);
			}
			else if (mShape == Circle)
			{
				XMVECTOR axis = XMVectorSet(1, 0, 0, 0); // 기준 방향
				XMStoreFloat3(&position, RANDOM::CircleEdgePoint(axis, 10.0f, &direction) + XMVectorSet(0, 10, 0, 0));
			}
			else
			{
				ASSERT(false);
			}

			mParticleData[i].MaxLifeTime = RANDOM::GetRandomValue(0.1f, 1.0f);
			mParticleData[i].LifeTime = mParticleData[i].MaxLifeTime;
			mParticleData[i].Velocity = direction;
			mParticles[i].Position = position;
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
