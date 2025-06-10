#include "Common.hlsl"

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};


float3 RandomNormal(float2 tex)
{
    float noiseX = (frac(sin(dot(tex, float2(15.8989f, 76.132f) * 1.0f)) * 46336.23745f));
    float noiseY = (frac(sin(dot(tex, float2(11.9899f, 62.223f) * 1.0f)) * 34748.34744f));
    float noiseZ = (frac(sin(dot(tex, float2(13.3238f, 63.232f) * 1.0f)) * 59998.47362f));

    return normalize(float3(noiseX, noiseY, noiseZ));
}

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // Quad covering screen in NDC space.
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
 
    // Transform quad corners to view space near plane.
    float4 ph = mul(vout.PosH, ProjectionInv);
    vout.PosV = ph.xyz / ph.w;

    return vout;
}
Texture2D DepthMap : register(t2);
Texture2D NormalMap : register(t3);
Texture2D NoiseMap : register(t4);
Texture2D PositionMap : register(t5);

static const float gSurfaceEpsilon = 0.015f;
static const float gOcclusionFadeEnd = 1.0f;
static const float gOcclusionFadeStart = 0.1f;
static const float gOcclusionRadius = 0.1f;


static const int NUM_SAMPLES = 16;
static const float3 sampleKernel[NUM_SAMPLES] =
{
    float3(0.2024537f, 0.8412041f, -0.9060141f),
    float3(-0.2200423f, 0.6282339f, -0.8275437f),
    float3(0.3677659f, 0.1086345f, -0.4468777f),
    float3(0.8775568f, 0.4617546f, -0.6427657f),
    float3(0.7867436f, -0.1414791f, -0.1567593f),
    float3(0.4893565f, -0.8253108f, -0.1563844f),
    float3(0.4401554f, -0.4228428f, -0.3300181f),
    float3(0.0011931f, -0.8048453f, 0.0726584f),
    float3(-0.7578573f, -0.5583061f, 0.2347527f),
    float3(-0.4540417f, -0.252365f, 0.0694318f),
    float3(-0.0489351f, -0.2522794f, 0.5924745f),
    float3(-0.4193292f, 0.2820481f, 0.6832647f),
    float3(-0.8433998f, 0.1451271f, 0.2269872f),
    float3(-0.4037157f, -0.6253081f, 0.6364237f),
    float3(-0.6657394f, 0.6296575f, 0.5249437f),
    float3(-0.0011783f, 0.2834622f, 0.8343929f)
};

float OcclusionFunction(float distZ)
{    
    float occlusion = 0.0f;
    if (distZ > gSurfaceEpsilon)
    {
        float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;

        occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
    }
	
    return occlusion;
}

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = gProj[3][2] / (z_ndc - gProj[2][2]);
    return viewZ;
}
 
float4 PS(VertexOut pin) : SV_Target
{
    float3 normal = normalize(NormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);
    float pz = DepthMap.SampleLevel(gsamDepth, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);

    float3 position = PositionMap.SampleLevel(gsamLinear, pin.TexC, 0); //(pz / pin.PosV.z) * pin.PosV;

    float3 randVec = 2.0f * NoiseMap.SampleLevel(gsamLinear, float2(ScreenWidth / 4.0f, ScreenHeight / 4.0f) * pin.TexC, 0.0f).rgb - 1.0f;
    
        // TBN »ý¼º
    float3 tangent = normalize(randVec - normal * dot(randVec, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    float occlusionSum = 0.0f;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float3 sampleVec = mul(TBN, sampleKernel[i]);
        float3 q = position + gOcclusionRadius * sampleVec;

        float4 projQ = mul(float4(q, 1.0f), ProjectionTex);
        projQ /= projQ.w;

        
        float rz = DepthMap.SampleLevel(gsamDepth, projQ.xy, 0.0f).r;
        rz = NdcDepthToViewDepth(rz);
        float3 r = (rz / q.z) * q;

        float3 diff = r - position;
        float dist2 = dot(diff, diff);
        float dp = max(dot(normal, normalize(diff)), 0.0f);
       
        float occlusion = dp;
        occlusionSum += occlusion;
    }
	
        
    occlusionSum /= NUM_SAMPLES;
	
    float access = 1.0f - occlusionSum;
    
    return saturate(pow(access, 1.5f));
    
}
