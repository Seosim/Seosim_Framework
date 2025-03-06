//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

Texture2D<float4> gMSAAMap : register(t0);

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

//ACES TONE MAPPING
static const float3x3 aces_input_matrix =
{
    float3(0.59719f, 0.35458f, 0.04823f),
	float3(0.07600f, 0.90834f, 0.01566f),
	float3(0.02840f, 0.13383f, 0.83777f)
};

static const float3x3 aces_output_matrix =
{
    float3(1.60475f, -0.53108f, -0.07367f),
	float3(-0.10208f, 1.10813f, -0.00605f),
	float3(-0.00327f, -0.07276f, 1.07602f)
};

float3 MultiplyMatrix(float3x3 m, float3 v)
{
    float x = m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2];
    float y = m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2];
    float z = m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2];

    return float3(x, y, z);
}

float3 RttAndOdtFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 ACESFitted(float3 v)
{
    v = MultiplyMatrix(aces_input_matrix, v);
    v = RttAndOdtFit(v);
    return mul(aces_output_matrix, v);
}


VertexOut VS(uint id : SV_VertexID)
{
    VertexOut output = (VertexOut) 0;
    
    output.UV = Vertices[id];
    output.Position = float4(output.UV.x * 2.0f - 1.0f, 1.0f - 2.0f * output.UV.y, 0.0f, 1.0f);
    
    return output;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 color = gMSAAMap.Sample(gsamLinear, pin.UV);
    
    //color.rgb = ACESFitted(color.rgb);
    
    //color = ToSRGB(color);
    
    return color;
}