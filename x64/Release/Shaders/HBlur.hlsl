Texture2D<float4> gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

static const int gBlurRadius = 3;

#define N 8
#define KERNEL_SIZE 7
#define CacheSize (8 + 2 * gBlurRadius)

static const float BlurWeights[KERNEL_SIZE] = { 0.05, 0.1, 0.15, 0.4, 0.15, 0.1, 0.05 }; // 반지름 1에 맞춘 가중치
groupshared float4 gCache[CacheSize];

[numthreads(8, 1, 1)]
void CS(int3 groupThreadID : SV_GroupThreadID, uint3 dispatchThreadID : SV_DispatchThreadID)
{ 
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
    }

    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0, 0, 0, 0);
	
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + gBlurRadius + i;
		
        blurColor += BlurWeights[i + gBlurRadius] * gCache[k];
    }
	
    gOutput[dispatchThreadID.xy] = blurColor;
    
    //////////////////////
    
    //uint2 uv = dispatchThreadID.xy;
    
    //uint2 texSize;
    //gInput.GetDimensions(texSize.x, texSize.y);
    
    //uint2 outputSize;
    //gOutput.GetDimensions(outputSize.x, outputSize.y);
    
    //// 출력 텍스처의 해상도에 맞는 입력 텍스처의 좌표 계산
    //float2 scale = float2(texSize) / float2(outputSize);
    //float2 inputCoord = (float2(uv) + 0.5) * scale; // 중앙 정렬
    
    //// 블러 처리
    //float4 blurredColor = float4(0, 0, 0, 0);
    //float totalWeight = 0.0;

    //// 수평 블러
    //for (int i = -BlurRadius; i <= BlurRadius; i++)
    //{
    //    int2 sampleCoord = int2(round(inputCoord.x + i), round(inputCoord.y));
    //    sampleCoord.x = clamp(sampleCoord.x, 0, texSize.x - 1);

    //    float weight = BlurWeights[i + BlurRadius];

    //    blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight;
    //    totalWeight += weight;
    //}

    //// 정규화하여 블러 적용
    //gOutput[uv] = blurredColor / totalWeight;
}