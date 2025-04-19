// Include common HLSL code.

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

cbuffer DownscaleSettings : register(b0)
{
    int DownscaleFactor;
}


[numthreads(32, 32, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);
    
    uint2 downsampledCoord = dispatchThreadID.xy;
    uint2 srcCoord = downsampledCoord * DownscaleFactor;

    if (srcCoord.x + DownscaleFactor - 1 >= texSize.x || srcCoord.y + DownscaleFactor - 1 >= texSize.y)
        return;

    float4 sum = 0;
    for (int y = 0; y < DownscaleFactor; y++)
    {
        for (int x = 0; x < DownscaleFactor; x++)
        {
            sum += gInput.Load(int3(srcCoord + uint2(x, y), 0));
        }
    }

    gOutput[downsampledCoord] = sum * (1.0 / (DownscaleFactor * DownscaleFactor));
}