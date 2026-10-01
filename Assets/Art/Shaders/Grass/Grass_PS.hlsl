// Grass 用 ピクセルシェーダー
// NOTE: ユーザー定数バッファは b4 (b0～b3 は DxLib が使う)。Grass_VS.hlsl と一致させる
cbuffer GrassBuffer : register(b4)
{
    float4 wind;
    float4 windDirection;
    float4 baseColor;
    float4 tipColor;
    float4 lightDirection;
    float4 lightColor;     // w=ambient
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR0;
    float3 Normal   : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // 葉は裏からも見えるので、面の向きを区別しない
    float  lambert     = abs(dot(normalize(input.Normal), -normalize(lightDirection.xyz)));
    float  halfLambert = lambert * 0.5f + 0.5f;
    float  ambient     = lightColor.w;
    float3 lighting    = ambient + lightColor.rgb * halfLambert * (1.0f - ambient);

    return float4(input.Color * lighting, 1.0f);
}
