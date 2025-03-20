//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

Texture2D gDiffuseMap : register(t2);

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 UV : UV;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Transform to world space.
    vout.PosH = mul(mul(mul(float4(vin.PosL, 1.0f), gWorld), ShadowView), ShadowProj);

    // Transform to homogeneous clip space.
    //vout.PosH = mul(mul(posW, gView), gProj);
    vout.UV = vin.UV;
	
    return vout;
}

void PS(VertexOut pin)
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
    clip(diffuseAlbedo.a - 0.1f);
}



