//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Normal = vin.Normal;
    vout.UV = vin.UV;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(pin.UV.x, pin.UV.y, 0, 1);
}


