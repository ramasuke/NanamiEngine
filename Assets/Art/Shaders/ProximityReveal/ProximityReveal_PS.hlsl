// ProximityReveal コンポーネントが毎フレーム書き込む定数バッファ (b1)
cbuffer ProximityBuffer : register(b1)
{
    float3 playerWorldPos;
    float  revealRadius;      // この距離以内は alpha = 1.0
    float  transitionWidth;   // revealRadius から +transitionWidth の範囲でフェード
    float3 _pad;
};

Texture2D    diffuseTex : register(t0);
SamplerState diffuseSmp : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float dist  = distance(input.WorldPos, playerWorldPos);
    float alpha = 1.0f - smoothstep(revealRadius, revealRadius + transitionWidth, dist);

    // 完全透明ピクセルを早期破棄してオーバードローを削減
    clip(alpha - 0.001f);

    float4 color = diffuseTex.Sample(diffuseSmp, input.TexCoord);
    color.a     *= alpha;
    return color;
}
