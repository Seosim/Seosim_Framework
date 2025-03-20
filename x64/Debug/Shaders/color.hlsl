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
    float4 ShadowPosH : POSITION1;
    float2 UV : UV;
};

struct PixelOut
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

	// Transform to clip space.
    vout.PosH = mul(mul(mul(float4(vin.PosL, 1.0f), gWorld), gView), gProj);

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Transform normal to world space.
    vout.NormalW = normalize(mul(float4(vin.Normal, 0.0f), gWorld).xyz);
    
    vout.ShadowPosH = mul(posW, ShadowTransform);

    vout.UV = vin.UV;

    return vout;
}

PixelOut PS(VertexOut pin)
{
    PixelOut pixelOut;
    
    // ��ǻ�� �� ���ø�
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
    
    diffuseAlbedo = LinearizeColor(diffuseAlbedo);
    
    // Normalized vectors
    float3 N = normalize(pin.NormalW); // ǥ�� ���� ����
    float3 L = -normalize(lightDir); // ���� ����
    float3 V = normalize(cameraPos - pin.PosW); // ī�޶� ���� (�� ����)
    float3 H = normalize(L + V); // Halfway ���� (Blinn-Phong)

    // Ambient term
    float3 ambient = diffuseAlbedo.rgb;

    // Diffuse term
    float diff = max(dot(N, L), 0.0);
    float3 diffuse = diff * diffuseAlbedo.rgb;

    // Specular term (Blinn-Phong)
    float shininess = 64.0; // ���� ����
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float3 specular = spec * lightColor;

    // �׸��� ���
    float shadowFac = CalcShadowFactor(pin.ShadowPosH);

    // Fresnel ȿ�� ��� (Fresnel-Schlick Approximation)
    float F0 = 0.04; // �⺻ �ݻ���
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - dot(V, N), 5);

    // ȯ�� �� ���ø�
    float3 R = reflect(-V, N);
    float3 envColor = gCubeMap.Sample(gsamLinear, R).rgb;

    // Fresnel ȿ���� �ݿ��Ͽ� ȯ�� �� ���� ����
    float3 finalEnvColor = envColor * fresnel;

    // ���� ���� ����
    float3 finalColor = ambient + (diffuse + specular) * shadowFac + finalEnvColor * 0.2f;

    pixelOut.color = float4(finalColor, 1.0f);
    
    pixelOut.normal = float4(mul(pin.NormalW, (float3x3) gView), 1.0f);
    
    
    return pixelOut;
}
