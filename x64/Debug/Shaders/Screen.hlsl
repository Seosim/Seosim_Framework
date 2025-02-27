//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

Texture2D gMSAAMap : register(t0);

static const float2 Vertices[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 UV : UV;
};

VertexOut VS(uint id : SV_VertexID)
{
    VertexOut output = (VertexOut) 0;
    
    output.UV = Vertices[id];
    output.Position = float4(output.UV.x * 2.0f - 1.0f, 1.0f - 2.0f * output.UV.y, 0.0f, 1.0f);
    
    return output;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gMSAAMap.Sample(gsamLinear, pin.UV);
}
