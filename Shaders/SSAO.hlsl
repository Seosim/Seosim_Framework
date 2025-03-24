#include "Common.hlsl"

static const float2 Vertices[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
};

struct Output
{
    float4 Position : SV_POSITION;
    float2 UV : UV;
    float3 PosV : TEXCOORD0;
};

Output VS(uint id : SV_VertexID)
{
    Output output = (Output) 0;
    
    output.UV = Vertices[id];
    output.Position = float4(output.UV.x * 2.0f - 1.0f, 1.0f - 2.0f * output.UV.y, 0.0f, 1.0f);
    
    float4 ph = mul(output.Position, ProjectionInv);
    output.PosV = ph.xyz / ph.w;
    
    return output;
}

Texture2DMS<float> DepthMap : register(t2);
Texture2DMS<float4> NormalMap : register(t3);
Texture2D NoiseMap : register(t4);

struct Input
{
    float4 Position : SV_POSITION;
    float2 UV : UV;
    float3 PosV : TEXCOORD0;
};

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

float4 PS(Input input) : SV_TARGET0
{
    const int NUM_SAMPLES = 14;
    const float3 sampleKernel[NUM_SAMPLES] =
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
    
    
    const float gOcclusionRadius = 0.5f; // 기본 오클루전 반경
    float dynamicRadius = gOcclusionRadius * (input.PosV.z / 0.05f); // 거리 기반 반경 조정

    // UV 좌표와 화면 크기에 맞는 노이즈 스케일링 적용
    float2 clipUV = input.Position.xy;
    float3 normal = normalize(NormalMap.Load(int2(int(clipUV.x), int(clipUV.y)), 0).xyz);
    float pz = DepthMap.Load(int2(int(clipUV.x), int(clipUV.y)), 0).r;
    // 깊이값을 뷰 공간으로 변환
    pz = NdcDepthToViewDepth(pz);
    float3 p = (pz / input.PosV.z) * input.PosV;
    
    float2 noiseScale = float2(ScreenWidth / 4.0f, ScreenHeight / 4.0f);
    float3 randVec = 2.0f * NoiseMap.SampleLevel(gsamLinear, input.UV * noiseScale, 0.0f).rgb - 1.0f;

    // 오클루전 계산을 위한 샘플링
    float occlusionSum = 0.0f;
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float3 offset = reflect(sampleKernel[i].xyz, randVec);
        float flip = sign(dot(offset, normal));
        float3 q = p + flip * dynamicRadius * offset;
        
        // 투영 행렬로 변환 및 클립 좌표 계산
        float4 projQ = mul(float4(q, 1.0f), ProjectionTex);
        projQ /= projQ.w;

        float2 clipProjQ = projQ.xy;
        clipProjQ.x *= ScreenWidth;
        clipProjQ.y *= ScreenHeight;
        
        // 비교 샘플 깊이값을 뷰 공간으로 변환
        float rz = DepthMap.Load(int2(int(clipProjQ.x), int(clipProjQ.y)), 0).r;
        rz = NdcDepthToViewDepth(rz);
        float3 r = (rz / q.z) * q;

        // 깊이 차이 및 노멀에 따른 오클루전 계산
        float distZ = p.z - r.z;
        float dp = max(dot(normal, normalize(r - p)), 0.0f);
        float occlusion = dp * OcclusionFunction(distZ, dynamicRadius);

        occlusionSum += occlusion;
    }
    
    occlusionSum /= NUM_SAMPLES;
    float access = 1.0f - occlusionSum;
    float ssao = saturate(pow(access, 2.0f));
    
    return float4(ssao, ssao, ssao, 1.0f);
}
