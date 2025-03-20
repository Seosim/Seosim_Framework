Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define KERNEL_SIZE 5 // 블러 커널 크기

static const int BlurRadius = 2;
//static const float BlurWeights[KERNEL_SIZE] = { 0.06136, 0.24477, 0.38774, 0.24477, 0.06136 };
static const float BlurWeights[KERNEL_SIZE] = { 0.15, 0.3, 0.1, 0.3, 0.15 };

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;
    
    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);
    
    uint2 outputSize;
    gOutput.GetDimensions(outputSize.x, outputSize.y);
    
    if (uv.x - 2 >= texSize.x || uv.y +  - 2 >= texSize.y)
        return;
    
    // 출력 텍스처의 해상도에 맞는 입력 텍스처의 좌표 계산
    float2 scale = float2(texSize) / float2(outputSize); // 입력 텍스처 크기와 출력 텍스처 크기의 비율
    float2 inputCoord = float2(uv) * scale; // 출력 좌표를 입력 좌표로 변환
    
    // 경계를 넘지 않도록 클램프
    inputCoord = clamp(inputCoord, float2(0, 0), float2(texSize.x - 1, texSize.y - 1));

    // 블러 처리
    float4 blurredColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0; // 정규화를 위한 가중치 합

    // 수직으로 인접한 픽셀들을 샘플링 (가우시안 블러 적용)
    for (int i = -BlurRadius; i <= BlurRadius; i++)
    {
        // 수직 방향으로 샘플링
        int2 sampleCoord = int2(inputCoord.x, inputCoord.y + i);
        sampleCoord.y = clamp(sampleCoord.y, 0, texSize.y - 1); // 경계를 넘지 않도록 클램프

        float weight = BlurWeights[i + BlurRadius]; // 가중치 적용
        blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight; // Load 함수로 텍스처 샘플링
        totalWeight += weight;
    }

    // 정규화하여 블러 적용
    gOutput[uv] = blurredColor / totalWeight;
}