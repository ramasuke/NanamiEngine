// LatticeBarrier 用 ピクセルシェーダー
// QuadRendererの板ポリゴンUV(0..1)を格子状(lattice)パターンに変換し、
// パルス明滅する発光ラインとして描画する。テクスチャは使わず完全にプロシージャル。
// gridScaleを大きくするほど格子の目が細かくなる(UV1あたりの格子セル数)。

cbuffer LatticeBarrierBuffer : register(b4)
{
    float  time;           // 経過時間(秒) - パルス明滅用
    float  gridScale;      // UV1あたりの格子セル数
    float  lineThickness;  // 格子線の太さ (0..1、セル内比率)
    float  pulseSpeed;     // 明滅速度
    float4 lineColor;      // 格子線の色 (rgb + alpha)
    float4 baseColor;      // セル内部(背景)の色 (rgb + alpha)
    float3 playerWorldPos; // プレイヤーのワールド座標
    float  visibleRadius;  // この距離以内は完全に見える
    float  fadeWidth;      // visibleRadiusから+fadeWidthの範囲でフェードイン/アウト
    float3 _pad;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    const float2 uv   = input.TexCoord * max(gridScale, 0.0001f);
    const float2 cell = frac(uv);
    const float2 distToLine = min(cell, 1.0f - cell);

    // fwidthベースでセル境界をアンチエイリアスする
    const float2 aa = fwidth(uv) * 1.5f + 1e-5f;
    const float2 lineMask2 = 1.0f - smoothstep(lineThickness, lineThickness + aa, distToLine);
    const float  lineMask  = saturate(max(lineMask2.x, lineMask2.y));

    const float pulse = 0.65f + 0.35f * sin(time * pulseSpeed);

    float4 color = lerp(baseColor, lineColor, lineMask);
    color.rgb *= pulse;
    color.a   *= pulse;

    // プレイヤーがvisibleRadius以内に近づいた時だけ見える(遠いと完全に透明)
    const float distToPlayer   = distance(input.WorldPos, playerWorldPos);
    const float proximityAlpha = 1.0f - smoothstep(visibleRadius, visibleRadius + fadeWidth, distToPlayer);
    color.a *= proximityAlpha;

    // 完全透明ピクセルを早期破棄してオーバードローを削減
    clip(color.a - 0.001f);
    return color;
}
