Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

[numthreads(32, 32, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 outputSize;
    gOutput.GetDimensions(outputSize.x, outputSize.y);
    
    uint2 inputSize;
    gInput.GetDimensions(inputSize.x, inputSize.y);
    
    uint downscaleFactorX = inputSize.x / outputSize.x;
    uint downscaleFactorY = inputSize.y / outputSize.y;

    uint2 downsampledCoord = dispatchThreadID.xy;
    uint2 srcCoord = downsampledCoord * uint2(downscaleFactorX, downscaleFactorY);

    float4 sum = 0;
    for (int y = 0; y < downscaleFactorY; y++)
    {
        for (int x = 0; x < downscaleFactorX; x++)
        {
            sum += gInput.Load(int3(srcCoord + uint2(x, y), 0));
        }
    }

    gOutput[downsampledCoord] = sum * (1.0 / (downscaleFactorX * downscaleFactorY));
}
