cbuffer cbSsao : register(b0)
{
    float4 gBlurWeights[3];
    float2 gInvRenderTargetSize;
};

// Input textures
Texture2D gNormalMap : register(t0);
Texture2D gDepthMap : register(t1);
Texture2D gInputMap : register(t2);

// Output texture
RWTexture2D<float4> gOutputMap : register(u0);

static float BlurWeights[12] =
{
    0.0221905,
    0.045589,
    0.0798114,
    0.119065,
    0.151361,
    0.163967,
    0.151361,
    0.119065,
    0.0798114,
    0.045589,
    0.0221905,
    0.0
};

[numthreads(1, 16, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    int2 texCoord = int2(DTid.xy);
    int width, height;
    gInputMap.GetDimensions(width, height);
    
    //if (texCoord.x >= width || texCoord.y >= height)
    //    return;
    
    static const int gBlurRadius = 5;
    
    float4 color = BlurWeights[gBlurRadius] * gInputMap.Load(int3(texCoord, 0)).r;
    float totalWeight = BlurWeights[gBlurRadius]; 
    
    float3 centerNormal = gNormalMap.Load(int3(texCoord, 0)).xyz;
    float centerDepth = gDepthMap.Load(int3(texCoord, 0)).r;
    
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        if (i == 0)
            continue;
        
        int2 offsetCoord = texCoord + int2(i, 0);
        if (offsetCoord.y < 0 || offsetCoord.y >= height)
            continue;
        
        float3 neighborNormal = gNormalMap.Load(int3(offsetCoord, 0)).xyz;
        float neighborDepth = gDepthMap.Load(int3(offsetCoord, 0)).r;
        
        if (dot(neighborNormal, centerNormal) >= 0.8f && abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = BlurWeights[i + gBlurRadius];
            color += weight * gInputMap.Load(int3(offsetCoord, 0)).r;
            totalWeight += weight;
        }
    }
    
    gOutputMap[texCoord] = color / totalWeight;
}