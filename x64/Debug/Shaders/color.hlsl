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
    float3 PosW : POSITION;
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
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Transform normal to world space.
    vout.NormalW = normalize(mul(float4(vin.Normal, 0.0f), gWorld).xyz);

    vout.UV = vin.UV;

    return vout;
}

PixelOut PS(VertexOut pin)
{
    PixelOut pixelOut;
    
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.UV);
    
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
    float3 finalColor = ambient + diffuse + specular;

    pixelOut.color = float4(finalColor, 1.0f);
    //pixelOut.normal = float4(N, 1.0f);
    pixelOut.normal = float4(mul(pin.NormalW, (float3x3) gView), 1.0f);
    return pixelOut;
}



