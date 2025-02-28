
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