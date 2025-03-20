Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

float GetBloomCurve(float x)
{
    float result = x;
    x *= 2.0f;
    
    result = x * 0.05f + max(0, x - 1.26f) * 0.5f;
    return result * 0.5f;
}

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;
    
    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);

    if (uv.x >= texSize.x || uv.y >= texSize.y)
        return;
    
    float4 color = gInput.Load(int3(uv, 0));

    float luminance = dot(color.rgb, float3(0.3f, 0.3f, 0.3f));

    float bloomFactor = GetBloomCurve(luminance);

    float3 bloomColor = color.rgb * bloomFactor / luminance;
    
    //감마 보정 처리된 값 원상 복구
    //bloomColor = pow(bloomColor, 1 / 2.2f);

    gOutput[uv] = float4(bloomColor, 1.0);
}