// 원본 100x100 텍스처 (UAV 불가, Load 사용)
Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[numthreads(8, 8, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    uint2 outCoord = DTid.xy;
    uint2 outSize;
    outputTexture.GetDimensions(outSize.x, outSize.y);
    
    uint2 inSize;
    inputTexture.GetDimensions(inSize.x, inSize.y);

    // 정규화된 UV 좌표
    float2 uv = (outCoord + 0.5f) / outSize;
    
    // 원본 텍스처 좌표 변환
    float2 samplePos = uv * (inSize - 1);

    // 정수 좌표 및 보간 가중치 계산
    int2 p0 = (int2) samplePos; // 좌측 상단 픽셀 좌표
    int2 p1 = min(p0 + int2(1, 0), inSize - 1);
    int2 p2 = min(p0 + int2(0, 1), inSize - 1);
    int2 p3 = min(p0 + int2(1, 1), inSize - 1);

    float2 f = frac(samplePos); // 보간 가중치

    // Load를 사용한 직접 샘플링
    float4 c00 = inputTexture.Load(int3(p0, 0));
    float4 c10 = inputTexture.Load(int3(p1, 0));
    float4 c01 = inputTexture.Load(int3(p2, 0));
    float4 c11 = inputTexture.Load(int3(p3, 0));

    // Bilinear 보간
    float4 c0 = lerp(c00, c10, f.x);
    float4 c1 = lerp(c01, c11, f.x);
    float4 finalColor = lerp(c0, c1, f.y);

    // 결과 저장
    outputTexture[outCoord] = finalColor;
}