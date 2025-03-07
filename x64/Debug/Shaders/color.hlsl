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

//PixelOut PS(VertexOut pin)
//{
//    PixelOut pixelOut;
    
//    // 텍스처에서 알베도 값을 샘플링
//    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
//    diffuseAlbedo = LinearizeColor(diffuseAlbedo);

//    // 노멀, 조명 및 뷰 벡터 계산
//    float3 N = normalize(pin.NormalW);
//    float3 L = normalize(lightDir);
//    float3 V = normalize(cameraPos - pin.PosW);
//    float3 H = normalize(L + V); // 블린-퐁에서 사용되는 Halfway 벡터

//    // 주변광 (Ambient) 계산
//    float3 ambient = BaseColor.rgb * 0.1f; // 기본 색상 기반의 약한 주변광

//    // 확산광 (Diffuse) 계산
//    float diff = max(dot(N, L), 0.0);
//    float3 diffuse = diff * diffuseAlbedo.rgb;

//    // 스펙큘러 (Specular) 계산 (Blinn-Phong)
//    float shininess = Smoothness * 128.0f; // 조도에 따른 하이라이트 강도
//    float spec = pow(max(dot(N, H), 0.0), shininess); // Halfway 벡터를 사용한 스펙큘러 계산
//    float3 specular = spec * lightColor * Metalic; // 금속성(Metallic)을 고려한 스펙큘러 반사

//    // 그림자 계산
//    float shadowFac = CalcShadowFactor(pin.ShadowPosH);

//    // 최종 색상 조합
//    float3 finalColor = ambient + (diffuse + specular) * shadowFac;

//    pixelOut.color = float4(finalColor, 1.0f); // 최종 색상 출력
//    pixelOut.normal = float4(mul(N, (float3x3) gView), 1.0f); // 뷰 공간 노멀
//    return pixelOut;
//}

PixelOut PS(VertexOut pin)
{
    PixelOut pixelOut;
    
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
   
    
    diffuseAlbedo = LinearizeColor(diffuseAlbedo);
    
    // Normalized vectors
    float3 N = normalize(pin.NormalW);
    float3 L = -normalize(lightDir);
    float3 V = normalize(cameraPos - pin.PosW);
    float3 H = normalize(L + V); // 블린-퐁에서 사용되는 Halfway 벡터

    // Ambient term
    float3 ambient = diffuseAlbedo.rgb;

    // Diffuse term
    float diff = max(dot(N, L), 0.0);
    float3 diffuse = diff * diffuseAlbedo.rgb;

    // Specular term (Blinn-Phong)
    float shininess = 64.0; // 조명 강도 조절 (값이 클수록 하이라이트가 선명해짐)
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float3 specular = spec * lightColor;

    // 그림자 계산
    float shadowFac = CalcShadowFactor(pin.ShadowPosH);

    // 최종 색상 조합
    float3 finalColor = ambient + (diffuse + specular) * shadowFac;
    
    pixelOut.color = float4(finalColor, 1.0f);
    
    //pixelOut.color = BaseColor;
    pixelOut.normal = float4(mul(pin.NormalW, (float3x3) gView), 1.0f);
    return pixelOut;
}