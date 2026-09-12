// Cloud 用 ピクセルシェーダー
// QuadRendererの板ポリゴン(頭上に水平配置)のWorldPos.xzを風速でスクロールしながら
// hashベースのvalue noiseを4オクターブ重ねたfBmでサンプルし、coverage/softnessの
// しきい値でスムーズステップしてアルファへ変換する。テクスチャは使わず完全にプロシージャル。
// 隙間は透明になり、背後のSkyDome/空が透けて見える。

cbuffer CloudBuffer : register(b4)
{
    float  time;        // 経過時間(秒) - 風スクロールの位相
    float  windX;        // ワールドX方向スクロール速度 (unit/sec)
    float  windZ;        // ワールドZ方向スクロール速度 (unit/sec)
    float  cloudScale;   // ワールド空間ノイズの周波数(大きいほど雲の粒が細かい)
    float  coverage;     // 0..1 しきい値。大きいほど空を覆う雲が増える
    float  softness;     // coverageしきい値のスムーズステップ幅(縁のやわらかさ)
    float  density;      // fBm出力の増幅/コントラスト
    float  _pad0;
    float4 cloudColor;   // rgb = 雲の色, a = 全体アルファ倍率
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

float hash(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float valueNoise(float2 p)
{
    const float2 i = floor(p);
    const float2 f = frac(p);

    const float a = hash(i);
    const float b = hash(i + float2(1.0, 0.0));
    const float c = hash(i + float2(0.0, 1.0));
    const float d = hash(i + float2(1.0, 1.0));

    const float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float fbm(float2 p)
{
    float sum  = 0.0;
    float amp  = 0.5;
    float freq = 1.0;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        sum  += amp * valueNoise(p * freq);
        freq *= 2.0;
        amp  *= 0.5;
    }
    return sum;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    const float2 windOffset = float2(windX, windZ) * time;
    const float2 samplePos  = input.WorldPos.xz * max(cloudScale, 0.0001f) + windOffset;

    const float n             = fbm(samplePos) * max(density, 0.0001f);
    const float coverageMask  = smoothstep(coverage - softness, coverage + softness, n);

    float4 color = cloudColor;
    color.a *= coverageMask;

    // 完全透明ピクセルを早期破棄してオーバードローを削減
    clip(color.a - 0.001f);
    return color;
}
