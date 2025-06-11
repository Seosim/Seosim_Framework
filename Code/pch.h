#pragma once
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <string>
#include <xstring>
#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <fstream>
#include <functional>
#include <chrono>
#include <random>
#include <bitset>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <D3Dcompiler.h>
#include "./Core/DDSTextureLoader12.h"
#include "./Core/d3dx12.h"


using namespace DirectX;
//using namespace DirectX::PackedVector;

#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)

#if !defined(RELEASE)
#define RELEASE(x) \
	(((x) == nullptr) || (delete (x), false) || ((x) = nullptr, false))
#endif

#if !defined(RELEASE_ARRAY)
#define RELEASE_ARRAY(x) \
	(((x) == nullptr) || (delete[] (x), false) || ((x) = nullptr, false))
#endif

#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)

#if !defined(DEBUG_LOG)
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#endif

#if !defined(DEBUG_BREAK)
#define DEBUG_BREAK() __debugbreak()
#endif

#if !defined(ASSERT)
#define ASSERT(expr) \
	((expr) || (DEBUG_LOG("%s, %s(%d)\n", (#expr), __FILE__, __LINE__), false) || (DEBUG_BREAK(), false))
#endif

#if !defined(MASSERT)
#define MASSERT(expr, msg) \
	((expr) || (DEBUG_LOG("%s, %s, %s(%d)\n", (msg), (#expr), __FILE__, __LINE__), false) || (DEBUG_BREAK(), false))
#endif

#if !defined(WARN)
#define WARN(expr) \
	((expr) || (DEBUG_LOG("%s, %s(%d)\n", (#expr), __FILE__, __LINE__), false))
#endif

#if !defined(MWARN)
#define MWARN(expr, msg) \
	((expr) || (DEBUG_LOG("%s, %s, %s(%d)\n", (msg), (#expr), __FILE__, __LINE__), false))
#endif

#else
#if !defined(DEBUG_LOG)
#define DEBUG_LOG(...) ((void)0)
#endif

#if !defined(DEBUG_BREAK)
#define DEBUG_BREAK() ((void)0)
#endif

#if !defined(ASSERT)
#define ASSERT(expr) ((void)0)
#endif

#if !defined(MASSERT)
#define MASSERT(expr, msg) ((void)0)
#endif

#if !defined(WARN)
#define WARN(expr) ((void)0)
#endif

#if !defined(MWARN)
#define MWARN(expr, msg) ((void)0)
#endif

#endif

#if !defined(RELEASE_COM)
#define RELEASE_COM(x) \
	(((x) == nullptr) || ((x)->Release(), false) || ((x) = nullptr, false))
#endif

#if !defined(HR)
#define HR(x) \
	{ \
		HRESULT hr = (x); \
		if (FAILED(hr)) \
		{ \
			LPVOID errorLog = nullptr; \
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, \
				nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
				reinterpret_cast<LPTSTR>(&errorLog), 0, nullptr); \
			_tprintf(TEXT("%s"), static_cast<TCHAR*>(errorLog)); \
			LocalFree(errorLog); \
			DEBUG_BREAK(); \
		} \
	} \
	((void)0)
#endif


namespace MATRIX {
	static XMFLOAT4X4 Identify4x4() {
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return I;
	}
}

namespace RANDOM {
	static XMVECTOR InsideUnitSphere() {
		std::random_device rd;
		std::uniform_real_distribution<float> uidF(-1.0f, 1.0f);
		std::default_random_engine dre(rd());

		XMVECTOR vec = XMVectorSet(uidF(dre), uidF(dre), uidF(dre), 0.0f);
		vec = XMVector3Normalize(vec);
		return vec;
	}

	static XMVECTOR DirectionInCone(XMVECTOR baseDir, float angleRadians)
	{
		std::random_device rd;
		std::default_random_engine dre(rd());
		std::uniform_real_distribution<float> distZ(cosf(angleRadians), 1.0f);
		std::uniform_real_distribution<float> distTheta(0.0f, XM_2PI);

		float z = std::cos(angleRadians);
		//float z = distZ(dre);
		float theta = distTheta(dre);

		float r = sqrtf(1.0f - z * z);
		float x = r * cosf(theta);
		float y = r * sinf(theta);

		XMVECTOR dirInCone = XMVectorSet(x, y, z, 0.0f);

		XMVECTOR defaultDir = XMVectorSet(0, 0, 1, 0);
		XMVECTOR rotationAxis = XMVector3Cross(defaultDir, XMVector3Normalize(baseDir));
		float angle = acosf(XMVectorGetX(XMVector3Dot(XMVector3Normalize(defaultDir), XMVector3Normalize(baseDir))));

		if (XMVector3Equal(rotationAxis, XMVectorZero()))
			return dirInCone;

		XMMATRIX rotation = XMMatrixRotationAxis(rotationAxis, angle);
		return XMVector3TransformNormal(dirInCone, rotation);
	}

	static float GetRandomValue(float minVal, float maxVal) {
		std::random_device rd;
		std::uniform_real_distribution<float> uidF(minVal, maxVal);
		std::default_random_engine dre(rd());

		return uidF(dre);
	}

	static XMVECTOR CircleEdgePoint(XMVECTOR axis, const float radius, XMVECTOR* direction = nullptr) {
		std::random_device rd;
		std::default_random_engine dre(rd());
		std::uniform_real_distribution<float> distTheta(0.0f, XM_2PI);

		float theta = distTheta(dre);

		XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		if (fabsf(XMVectorGetX(XMVector3Dot(axis, up))) > 0.99f)
			up = XMVectorSet(1, 0, 0, 0);

		XMVECTOR u = XMVector3Normalize(XMVector3Cross(axis, up));
		XMVECTOR v = XMVector3Normalize(XMVector3Cross(axis, u));

		XMVECTOR pointOffset = radius * (cosf(theta) * u + sinf(theta) * v);

		if (nullptr != direction)
		{
			float theta2 = theta + XMConvertToRadians(15.0f);
			XMVECTOR pointOffset2 = radius * (cosf(theta2) * u + sinf(theta2) * v);
			XMVECTOR dir = XMVector3Normalize(pointOffset2 - pointOffset);
			*direction = dir;
		}

		return pointOffset;
	}
}

namespace MathTool {
	static float Clamp(float val, float minVal, float maxVal)
	{
		return std::max(std::min(maxVal, val), minVal);
	}

	constexpr float PI = 3.141597f;
}

constexpr UINT MAX_TEXTURE_COUNT = 8;

constexpr UINT MSAA_SAMPLING_COUNT = 4;

constexpr UINT SHADOW_MAP_SIZE = 4096;

enum class eDescriptorRange {
	CUBE_MAP,
	SHADOW_MAP,
	TEXTURE0,
	TEXTURE1,
	TEXTURE2,
	TEXTURE3,
	COUNT
};

enum class eRootParameter {
	OBJECT,
	MATERIAL,
	LIGHT,
	CAMERA,
	SHADOW,
	CUBE_MAP,
	SHADOW_TEXTURE,
	TEXTURE0,
	TEXTURE1,
	TEXTURE2,
	TEXTURE3,
	COUNT
};

enum class eComputeRootParamter {
	INPUT0,
	OUTPUT,
	INPUT1,
	DOWN_SCALE_SIZE,
	CAMERA_N_TIMER,
	INPUT2,
	INPUT3,
	COUNT
};