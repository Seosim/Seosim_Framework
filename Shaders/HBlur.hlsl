Texture2D<float4> gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define KERNEL_SIZE 3
static const int BlurRadius = 1;
static const float BlurWeights[KERNEL_SIZE] = { 0.25, 0.5, 0.25 }; // ������ 1�� ���� ����ġ

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 uv = dispatchThreadID.xy;
    
    uint2 texSize;
    gInput.GetDimensions(texSize.x, texSize.y);
    
    uint2 outputSize;
    gOutput.GetDimensions(outputSize.x, outputSize.y);
    
    // ��� �ؽ�ó�� �ػ󵵿� �´� �Է� �ؽ�ó�� ��ǥ ���
    float2 scale = float2(texSize) / float2(outputSize);
    float2 inputCoord = (float2(uv) + 0.5) * scale; // �߾� ����
    
    // �� ó��
    float4 blurredColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0;

    // ���� ��
    for (int i = -BlurRadius; i <= BlurRadius; i++)
    {
        int2 sampleCoord = int2(round(inputCoord.x + i), round(inputCoord.y));
        sampleCoord.x = clamp(sampleCoord.x, 0, texSize.x - 1);

        float weight = BlurWeights[i + BlurRadius];

        blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight;
        totalWeight += weight;
    }

    // ����ȭ�Ͽ� �� ����
    gOutput[uv] = blurredColor / totalWeight;
}