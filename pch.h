#pragma once

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <string>
#include <xstring>
#include <array>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <functional>


#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <D3Dcompiler.h>
#include "DDSTextureLoader12.h"

#include "d3dx12.h"


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

namespace MathTool {
	static float Clamp(float val, float minVal, float maxVal)
	{
		return max(min(maxVal, val), minVal);
	}

	constexpr float PI = 3.141597f;
}