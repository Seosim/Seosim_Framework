//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

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
    float3 cameraPos;
}

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

float4 PS(VertexOut pin) : SV_Target
{
    // Normalize vectors.
    float3 N = normalize(pin.NormalW);
    float3 L = lightDir;
    float3 V = normalize(cameraPos - pin.PosW);
    float3 R = reflect(-L, N);

    // Ambient term.
    float3 ambient = color.rgb;

    // Diffuse term.
    float diff = max(dot(N, L), 0.0);
    float3 diffuse = diff * color.rgb;

    // Specular term.
    float spec = 0.0;
    if (diff > 0.0)
    {
        spec = pow(max(dot(R, V), 0.0), 64);
    }
    float3 specular = spec * lightColor;

    // Combine results.
    float3 finalColor = ambient + diffuse + specular;

    return float4(finalColor, 1.0f);
}



