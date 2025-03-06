//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "Common.hlsl"

Texture2D gDiffuseMap : register(t0);

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
    
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
    
    diffuseAlbedo = LinearizeColor(diffuseAlbedo);
    
    //return diffuseAlbedo;
    
    // Normalize vectors.
    float3 N = normalize(pin.NormalW);
    float3 L = lightDir;
    float3 V = normalize(cameraPos - pin.PosW);
    float3 R = reflect(-L, N);

    // Ambient term.
    float3 ambient = diffuseAlbedo.rgb;

    // Diffuse term.
    float diff = max(dot(N, L), 0.0);
    float3 diffuse = diff * diffuseAlbedo.rgb;

    // Specular term.
    float spec = 0.0;
    if (diff > 0.0)
    {
        spec = pow(max(dot(R, V), 0.0), 32);
    }
    float3 specular = spec * lightColor;

    // Combine results.
    float shadowFac = CalcShadowFactor(pin.ShadowPosH);
    
    float3 finalColor = (ambient + diffuse + specular) * shadowFac;
    
    float4 test = pin.ShadowPosH / pin.ShadowPosH.w;
    pixelOut.color = float4(finalColor, 1.0f);
    //pixelOut.color = float4(shadowFac, shadowFac, shadowFac, 1.0f);
    //pixelOut.color = test;
    pixelOut.color = gShadowMap.Sample(gsamLinear, test.xy);
    //float shadowDepth = gShadowMap.SampleLevel(gsamLinear, test.xy, 0.0f).r;
    //pixelOut.color = gShadowMap.Sample(gsamLinear, pin.UV);
    pixelOut.normal = float4(mul(pin.NormalW, (float3x3) gView), 1.0f);
    return pixelOut;
}