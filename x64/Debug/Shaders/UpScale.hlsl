Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 outCoord = dispatchThreadID.xy;
    uint2 outSize;
    outputTexture.GetDimensions(outSize.x, outSize.y);
    
    uint2 inSize;
    inputTexture.GetDimensions(inSize.x, inSize.y);

    float2 uv = (outCoord + 0.5f) / outSize;
    
    float2 samplePos = uv * (inSize - 1);

    int2 p0 = (int2) samplePos;
    int2 p1 = min(p0 + int2(1, 0), inSize - 1);
    int2 p2 = min(p0 + int2(0, 1), inSize - 1);
    int2 p3 = min(p0 + int2(1, 1), inSize - 1);

    float2 f = frac(samplePos);

    float4 c00 = inputTexture.Load(int3(p0, 0));
    float4 c10 = inputTexture.Load(int3(p1, 0));
    float4 c01 = inputTexture.Load(int3(p2, 0));
    float4 c11 = inputTexture.Load(int3(p3, 0));

    float4 c0 = lerp(c00, c10, f.x);
    float4 c1 = lerp(c01, c11, f.x);
    float4 finalColor = lerp(c0, c1, f.y);

    outputTexture[outCoord] = finalColor;
}