
SamplerState gsamLinear : register(s0);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldViewProj;
};

cbuffer cbPerMaterial : register(b1)
{
    float4 color;
};

cbuffer cbPerLight : register(b2)
{
    float3 lightDir;
    float3 lightColor;
}

cbuffer cbPerCamera : register(b3)
{
    float4x4 gView;
    float4x4 gProj;
    float3 cameraPos;
    float ScreenWidth;
    float ScreenHeight;
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