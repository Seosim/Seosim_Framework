#pragma once

#include "pch.h"
#include "IComponent.h"
#include "Core/UploadBuffer.h"

class ParticleGenerator : public IComponent {
private:
	struct ParticleData {
		XMVECTOR Velocity;
		float Speed;
		float LifeTime;
		float MaxLifeTime;

		bool IsActive() const { return LifeTime > 0.0f; }
	};

	struct Particle {
		XMFLOAT3 Position;
		float LifeFactor;
		XMFLOAT2 Size;
	};
public:
	enum eShape {
		Sphere,
		Cone,
		Circle,
		Count
	};

	ParticleGenerator() = default;
	~ParticleGenerator() { }
	ParticleGenerator(const ParticleGenerator&) = delete;

	void Initialize(ID3D12Device* pDevice, const UINT particleCount, eShape shape);

	virtual void Awake() {}
	virtual void Update(const float deltaTime) override;

	void Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap);
private:
	UINT mCapacity = 50;
	UINT mAliveCount = 0;
	std::vector<Particle> mParticles = {};
	std::vector<ParticleData> mParticleData = {};

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {}; 

	std::unique_ptr<UploadBuffer> mParticleBuffer = nullptr;

	eShape mShape = eShape::Sphere;
};
