//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

Texture2D gDiffuseMap : register(t2);

cbuffer cbPerMaterial : register(b1)
{
    float4 BaseColor;
    float4 Emission;
    float Metalic;
    float Smoothness;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 PosW : POSITION0;
    float3 PosV : POSITION1;
    float4 ShadowPosH : POSITION2;
    float2 UV : UV;
};

struct PixelOut
{
    float4 color : SV_TARGET0;
    float4 position : SV_TARGET1;
    float4 normal : SV_TARGET2;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(mul(mul(float4(vin.PosL, 1.0f), gWorld), gView), gProj);

    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    float4 posV = mul(posW, gView);
    vout.PosV = posV.xyz;

    vout.NormalW = mul(vin.Normal, (float3x3)gWorld);
    
    vout.ShadowPosH = mul(posW, ShadowTransform);

    vout.UV = vin.UV;

    return vout;
}

static const float PI = 3.141592;

PixelOut PS(VertexOut pin)
{
    PixelOut pixelOut;

    float3 albedo = LinearizeColor(BaseColor).rgb;

    float3 N = normalize(pin.NormalW);
    float3 V = normalize(CameraPos - pin.PosW);
    float3 L = -normalize(lightDir);
    float3 H = normalize(L + V);

    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    float3 F0 = lerp(dielectricF0, albedo, Metalic);

    float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    float NdotL = max(dot(N, L), 0.0);

    float3 diffuse = (1.0 - F) * albedo / PI * lightColor;

    float roughness = 1.0 - Smoothness;
    float shininess = lerp(1.0, 256.0, Smoothness);
    float specularTerm = pow(max(dot(N, H), 0.0), shininess);
    float3 specular = F * specularTerm * lightColor;

    float3 R = reflect(-V, N);
    float3 envColor = gCubeMap.Sample(gsamLinear, R).rgb;
    float3 environment = lerp(envColor * albedo, envColor * F, Metalic);
    environment *= Smoothness;

    float shadowFac = CalcShadowFactor(pin.ShadowPosH);

    float3 ambient = albedo * 0.5f;

    float3 color = ambient + (diffuse + specular) * NdotL * shadowFac + environment * 0.2f;

    color += Emission.rgb;

    pixelOut.color = float4(color, 1.0f);
    pixelOut.position = float4(pin.PosV, 1.0f);
    pixelOut.normal = float4(mul(N, (float3x3) gView), 0.0f);

    return pixelOut;
}
