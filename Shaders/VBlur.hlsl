Texture2D<float4> gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define KERNEL_SIZE 7
static const int BlurRadius = 3;
static const float BlurWeights[KERNEL_SIZE] = {0.05, 0.1, 0.15, 0.4, 0.15, 0.1, 0.05 }; // 반지름 1에 맞춘 가중치

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;

    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);

    uint2 outputSize;
    gOutput.GetDimensions(outputSize.x, outputSize.y);

    // 입력 텍스처 좌표 변환
    float2 scale = float2(texSize) / float2(outputSize);
    float2 inputCoord = (float2(uv) + 0.5) * scale; // 중앙 정렬

    // 블러 결과 변수
    float4 blurredColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0;

    // 블러 적용 (수직 블러)
    for (int i = -BlurRadius; i <= BlurRadius; i++)
    {
        int2 sampleCoord = int2(round(inputCoord.x), round(inputCoord.y + i));
        sampleCoord.y = clamp(sampleCoord.y, 0, texSize.y - 1); // 경계 체크

        float weight = BlurWeights[i + BlurRadius];

        blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight;
        totalWeight += weight;
    }

    // 정규화 후 저장
    gOutput[uv] = blurredColor / totalWeight;
}