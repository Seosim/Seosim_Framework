
SamplerState gsamLinear : register(s0);
SamplerComparisonState gsamShadow : register(s1);
SamplerState gsamPointClamp : register(s2);
SamplerState gsamDepth : register(s3);


TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldViewProj;
};

cbuffer cbPerLight : register(b2)
{
    float3 lightDir;
    float padding0;
    float3 lightColor;
}

cbuffer cbPerCamera : register(b3)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 ProjectionTex;
    float4x4 ProjectionInv;
    float3 cameraPos;
    float ScreenWidth;
    float ScreenHeight;
    float ElapsedTime;
}

cbuffer cbPerShadow : register(b4)
{
    float4x4 ShadowView;
    float4x4 ShadowProj;
    float4x4 ShadowTransform;
    float3 LightDir;
    float NearZ;
    float3 LightPosW;
    float FarZ;
}

// 선형 공간으로 변환
float4 LinearizeColor(float4 color)
{
    float3 linearColor = color.rgb;
    return float4(pow(linearColor, 2.2f), color.a);
}

// 선형 공간에서 sRGB로 변환
float4 ToSRGB(float4 color)
{
    float3 srgbColor = color.rgb;
    return float4(pow(srgbColor, 1.0f / 2.2f), color.a);
}


//그림자 연산
float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    
    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };
    
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow, shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}