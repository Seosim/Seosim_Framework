Texture2D<float4> gNormalMap : register(t0);
Texture2D<float> gDepthMap : register(t1);
Texture2D<float> gInputMap : register(t2);

RWTexture2D<float4> gOutputMap : register(u0);

static const int gBlurRadius = 8;
static const float NormalThreshold = 0.8f;
static const float DepthThreshold = 0.2f;

static const float BlurWeights[17] =
{
    0.0081, 0.0169, 0.0321, 0.0561, 0.0902,
    0.1353, 0.1791, 0.2066, 0.2159, 0.2066,
    0.1791, 0.1353, 0.0902, 0.0561, 0.0321,
    0.0169, 0.0081
};

[numthreads(1, 16, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 texCoord = dispatchThreadID.xy;

    float centerValue = gInputMap[texCoord];
    float3 centerNormal = gNormalMap[texCoord].xyz;
    float centerDepth = gDepthMap[texCoord];

    float result = centerValue * BlurWeights[gBlurRadius];
    float totalWeight = BlurWeights[gBlurRadius];

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        if (i == 0)
            continue;

        int2 offsetCoord = texCoord + int2(0, i);
        
        // clamp instead of discard (branch 제거)
        offsetCoord = clamp(offsetCoord, int2(0, 0), int2(16384, 16384)); // 큰 값 설정

        float3 neighborNormal = gNormalMap[offsetCoord].xyz;
        float neighborDepth = gDepthMap[offsetCoord];

        float normalDot = dot(neighborNormal, centerNormal);
        float depthDiff = abs(neighborDepth - centerDepth);
        float valid = step(NormalThreshold, normalDot) * step(depthDiff, DepthThreshold); // 분기 제거

        float weight = BlurWeights[i + gBlurRadius] * valid;

        result += gInputMap[offsetCoord] * weight;
        totalWeight += weight;
    }

    gOutputMap[texCoord] = result / totalWeight;
}