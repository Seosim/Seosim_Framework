Texture2D<float4> gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define KERNEL_SIZE 7
static const int BlurRadius = 3;
static const float BlurWeights[KERNEL_SIZE] = {0.05, 0.1, 0.15, 0.4, 0.15, 0.1, 0.05 }; // ������ 1�� ���� ����ġ

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;

    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);

    uint2 outputSize;
    gOutput.GetDimensions(outputSize.x, outputSize.y);

    // �Է� �ؽ�ó ��ǥ ��ȯ
    float2 scale = float2(texSize) / float2(outputSize);
    float2 inputCoord = (float2(uv) + 0.5) * scale; // �߾� ����

    // �� ��� ����
    float4 blurredColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0;

    // �� ���� (���� ��)
    for (int i = -BlurRadius; i <= BlurRadius; i++)
    {
        int2 sampleCoord = int2(round(inputCoord.x), round(inputCoord.y + i));
        sampleCoord.y = clamp(sampleCoord.y, 0, texSize.y - 1); // ��� üũ

        float weight = BlurWeights[i + BlurRadius];

        blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight;
        totalWeight += weight;
    }

    // ����ȭ �� ����
    gOutput[uv] = blurredColor / totalWeight;
}