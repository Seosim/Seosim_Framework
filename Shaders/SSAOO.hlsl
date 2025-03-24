#include "Common.hlsl"

RWTexture2D<float4> SsaoOutput : register(u0);
Texture2DMS<float> DepthMap : register(t0);
Texture2DMS<float4> NormalMap : register(t1);
Texture2D NoiseMap : register(t2);

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
    return viewZ;
}

float OcclusionFunction(float distZ, float occlusionRadius)
{
    float attenuation = saturate(1.0f - (distZ / occlusionRadius));
    return pow(attenuation, 2.0f);
}

static const int NUM_SAMPLES = 14;
static const float3 sampleKernel[NUM_SAMPLES] =
{
    float3(0.144879371, 0.144879371, 0.144879371),
    float3(-0.388377190, -0.388377190, -0.388377190),
    float3(-0.228040740, 0.228040740, 0.228040740),
    float3(0.494532436, -0.494532436, -0.494532436),
    float3(0.397654027, 0.397654027, -0.397654027),
    float3(-0.352128685, -0.352128685, 0.352128685),
    float3(-0.296018183, 0.296018183, -0.296018183),
    float3(0.532300651, -0.532300651, 0.532300651),
    float3(-0.867130041, 0.00000000, 0.00000000),
    float3(0.809953570, 0.00000000, 0.00000000),
    float3(0.0, -0.380581081, 0.00000000),
    float3(0.0, 0.894207597, 0.00000000),
    float3(0.0, 0.00000000, -0.782876074),
    float3(0.0, 0.00000000, 0.635151207)
};

#define N 32

[numthreads(N, N, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    int SsaoMapWidth = ScreenWidth;
    int SsaoMapHeight = ScreenHeight;
    
    int NoiseMapWidth;
    int NoiseMapHeight;
    
    if (id.x >= SsaoMapWidth || id.y >= SsaoMapHeight)
        return;
    
    NoiseMap.GetDimensions(NoiseMapWidth, NoiseMapHeight);

    float2 pixelCoord;
    pixelCoord.x = id.x / (float)SsaoMapWidth * 2.0f - 1.0f;
    pixelCoord.y = id.y / (float)SsaoMapHeight * 2.0f - 1.0f;
    
    float3 normal = normalize(NormalMap.Load(int2(id.xy), 0).xyz);
    float pz = DepthMap.Load(int2(id.xy), 0).r;

    pz = NdcDepthToViewDepth(pz);
    
    float4 ph = mul(float4(pixelCoord, 0.0f, 1.0f), ProjectionInv);
    float3 posV = ph.xyz / ph.w;
    
    float3 p = (pz / posV.z) * posV;
    
    
    float occlusionSum = 0.0f;
    const float gOcclusionRadius = 0.5f;
    float dynamicRadius = gOcclusionRadius * (posV.z / 0.05f); // 거리 기반 반경 조정

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float3 randVec = 2.0f * NoiseMap.Load(int3(id.xy % 4 * float2(SsaoMapWidth, SsaoMapHeight) / 4, 0)).rgb - 1.0f;
        
        float3 offset = reflect(sampleKernel[i], randVec);
        float flip = sign(dot(offset, normal));
        float3 q = p + flip * dynamicRadius * offset;
        
        float4 projQ = mul(float4(q, 1.0f), ProjectionTex);
        projQ /= projQ.w;
        
        int2 clipProjQ = int2(projQ.xy * float2(SsaoMapWidth, SsaoMapHeight));
        
        float rz = DepthMap.Load(int2(clipProjQ), 0).r;
        rz = NdcDepthToViewDepth(rz);
        float3 r = (rz / q.z) * q;
        
        float distZ = p.z - r.z;
        float dp = max(dot(normal, normalize(r - p)), 0.0f);
        float occlusion = dp * OcclusionFunction(distZ, dynamicRadius);
        occlusionSum += occlusion;
    }
    
    occlusionSum /= NUM_SAMPLES;
    float ssao = saturate(pow(1.0f - occlusionSum, 2.0f));

    SsaoOutput[id.xy] = float4(ssao.rrr, 1.0f);
}