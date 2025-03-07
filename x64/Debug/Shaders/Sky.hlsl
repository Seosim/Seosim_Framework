//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================

// Include common HLSL code.

#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
    float2 UV : UV;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};
 
VertexOut VS(VertexIn vin)
{
	VertexOut vout;

    vout.PosL = vin.PosL;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	//// Use local vertex position as cubemap lookup vector.
	//vout.PosL = vin.PosL;
	
	//// Transform to world space.
	//float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	//// Always center sky about camera.
	//posW.xyz += cameraPos;

	//// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
 //   vout.PosH = mul(mul(posW, gView), gProj).xyww;
	
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = gCubeMap.Sample(gsamLinear, pin.PosL);
    return color;
    return LinearizeColor(color);
}

