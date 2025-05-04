//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

//Texture2D gDiffuseMap : register(t2);

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

    vout.PosH = mul(mul(mul(float4(vin.PosL, 1.0f), gWorld), ShadowView), ShadowProj);
    vout.UV = vin.UV;
	
    return vout;
}

void PS(VertexOut pin)
{
    
}



