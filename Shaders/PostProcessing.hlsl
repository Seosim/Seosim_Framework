//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================

// Include common HLSL code.

#include "Common.hlsl"

Texture2D gInput : register(t0);
Texture2D gBloomMap : register(t1);
Texture2D gSSAOMap : register(t2);
Texture2D gMaskMap : register(t3);
RWTexture2D<float4> gOutput : register(u0);

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

[numthreads(32, 32, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{        
    //TODO:
    int2 texCoord = dispatchThreadID.xy;

    // 원본 색상 가져오기
    float4 color = gInput[texCoord];
    float4 bloomColor = gBloomMap[texCoord];
    float ssao = gSSAOMap[texCoord];
    float mask = gMaskMap[texCoord];
    
    color *= saturate(ssao + mask);
    color += bloomColor;
    
    
    // ACES 톤 매핑 적용
    color.rgb = ACESFitted(color.rgb);
    // sRGB 변환 적용
    color.rgb = ToSRGB(color);
    
    // 결과를 UAV에 저장
    gOutput[texCoord] = color;
}
