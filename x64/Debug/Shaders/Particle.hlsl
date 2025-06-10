#include "Common.hlsl"

Texture2D gDepthMap : register(t2);
Texture2D gTexture: register(t3);

struct VertexIn
{
    float3 PosW : POSITION;
    float LifeFactor : FACTOR;
    float2 Size : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
    float LifeFactor : FACTOR;
};

struct PixelOut
{
    float4 Color : SV_Target0;
    float4 Mask : SV_Target1;
};

VertexIn VS(VertexIn vin)
{
    return vin;
}

[maxvertexcount(4)]
void GS(point VertexIn gin[1], uint primID : SV_PrimitiveID, inout TriangleStream<GeoOut> triStream)
{
    float3 look = normalize(CameraPos - gin[0].PosW);
    float3 right = cross(CameraUpAxis, look);

    float halfWidth = 0.5f * gin[0].Size.x * gin[0].LifeFactor;
    float halfHeight = 0.5f * gin[0].Size.y * gin[0].LifeFactor;

    float4 v[4];
    v[0] = float4(gin[0].PosW + halfWidth * right - halfHeight * CameraUpAxis, 1.0f);
    v[1] = float4(gin[0].PosW + halfWidth * right + halfHeight * CameraUpAxis, 1.0f);
    v[2] = float4(gin[0].PosW - halfWidth * right - halfHeight * CameraUpAxis, 1.0f);
    v[3] = float4(gin[0].PosW - halfWidth * right + halfHeight * CameraUpAxis, 1.0f);

    float2 texC[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    GeoOut gout;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(mul(v[i], gView), gProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.TexC = texC[i];
        gout.PrimID = primID;
        gout.LifeFactor = gin[0].LifeFactor;

        triStream.Append(gout);
    }
}

PixelOut PS(GeoOut pin)
{
    PixelOut pOut;
    
    float4 color1 = float4(10.0f, 1.0f, 1.0f, 1.0f);
    float4 color2 = float4(10.0f, 1.0f, 1.0f, 1.0f);
    color1.a = 1.0f;
    color2.a = 1.0f;
    
    pOut.Color = lerp(color1, color2, pin.LifeFactor);
    //pOut.Color = gTexture.Sample(gsamLinear, pin.TexC) * lerp(color1, color2, pin.LifeFactor);
    pOut.Mask = float4(1, 1, 1, 1);

    return pOut;
}
