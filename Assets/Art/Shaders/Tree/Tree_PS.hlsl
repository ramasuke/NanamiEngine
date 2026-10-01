// Tree 用 ピクセルシェーダー (葉のマテリアル専用)
// NOTE: ユーザー定数バッファは b4 (b0～b3 は DxLib が使う)。Tree_VS.hlsl と一致させる
cbuffer TreeWindBuffer : register(b4)
{
    float4 wind;
    float4 windDirection;
    float4 lightDirection;
    float4 lightColor;     // w=ambient
};

Texture2D    diffuseTex : register(t0);
SamplerState diffuseSmp : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 tex = diffuseTex.Sample(diffuseSmp, input.TexCoord);

    // 葉はアルファ抜きのカードなので、半透明ブレンドではなくアルファテストで抜く
    clip(tex.a - 0.5f);

    // 葉は裏からも見えるので、面の向きを区別しない
    float  lambert     = abs(dot(normalize(input.Normal), -normalize(lightDirection.xyz)));
    float  halfLambert = lambert * 0.5f + 0.5f;
    float  ambient     = lightColor.w;
    float3 lighting    = ambient + lightColor.rgb * halfLambert * (1.0f - ambient);

    return float4(tex.rgb * lighting, 1.0f);
}
