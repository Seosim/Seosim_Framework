Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

float GetBloomCurve(float x)
{
    float result = x;
    x *= 2.0f;
    
    float value = 0.16f;
    
    result = x * 0.05f + max(0, x - value) * 0.5f;
    return result * 0.5f;
}

static const float Threshold = 0.5f;
static const float ThresholdKnee = 0.1f;
static const float Intensity = 1.0f;

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;
    
    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);
    
    float4 color = gInput.Load(int3(uv, 0));
    
    
    float brightness = max(max(color.r, color.g), color.b);
    float softness = clamp(brightness - Threshold + ThresholdKnee, 0.0, 2.0 * ThresholdKnee);
    softness = (softness * softness) / (4.0 * ThresholdKnee + 1e-4);
    float multiplier = max(brightness - Threshold, softness) / max(brightness, 1e-4);
    color *= multiplier;
    color *= Intensity;

    //float luminance = dot(color.rgb, float3(0.15f, 0.15f, 0.15f));

    //float bloomFactor = GetBloomCurve(luminance);

    //float3 bloomColor = color.rgb * bloomFactor / luminance * 1.5f;

    gOutput[uv] = float4(color.rgb, 1.0);
}
