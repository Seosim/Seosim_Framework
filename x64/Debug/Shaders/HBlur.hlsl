Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define KERNEL_SIZE 5 // �� Ŀ�� ũ��

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
    
    // ��� �ؽ�ó�� �ػ󵵿� �´� �Է� �ؽ�ó�� ��ǥ ���
    float2 scale = float2(texSize) / float2(outputSize); // �Է� �ؽ�ó ũ��� ��� �ؽ�ó ũ���� ����
    float2 inputCoord = float2(uv) * scale; // ��� ��ǥ�� �Է� ��ǥ�� ��ȯ
    
    // ��踦 ���� �ʵ��� Ŭ����
    inputCoord = clamp(inputCoord, float2(0, 0), float2(texSize.x - 1, texSize.y - 1));

    // �� ó��
    float4 blurredColor = float4(0, 0, 0, 0);
    float totalWeight = 0.0; // ����ȭ�� ���� ����ġ ��

    // �������� ������ �ȼ����� ���ø� (����þ� �� ����)
    for (int i = -BlurRadius; i <= BlurRadius; i++)
    {
        // ���� �������� ���ø�
        int2 sampleCoord = int2(inputCoord.x + i, inputCoord.y);
        sampleCoord.x = clamp(sampleCoord.x, 0, texSize.x - 1); // ��踦 ���� �ʵ��� Ŭ����

        float weight = BlurWeights[i + BlurRadius]; // ����ġ ����
        blurredColor += gInput.Load(int3(sampleCoord, 0)) * weight; // Load �Լ��� �ؽ�ó ���ø�
        totalWeight += weight;
    }

    // ����ȭ�Ͽ� �� ����
    gOutput[uv] = blurredColor / totalWeight;
}