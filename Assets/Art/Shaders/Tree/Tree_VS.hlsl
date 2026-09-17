// Tree 用 頂点シェーダー (葉のマテリアル専用 / DxLib Direct3D 11 / ModelRenderer の MV1 剛体メッシュ)
//
// DxLib が頂点シェーダーへ渡す定数バッファ (DxShader_VS_D3D11.h / VertexShader.h):
//   b0 : DX_D3D11_CONST_BUFFER_COMMON              (ライト・マテリアル・フォグ)
//   b1 : DX_D3D11_VS_CONST_BUFFER_BASE             (射影・ビュー・ローカル→ワールド行列)
//   b2 : DX_D3D11_VS_CONST_BUFFER_OTHERMATRIX      (シャドウマップ行列・テクスチャ行列)
//   b3 : DX_D3D11_VS_CONST_BUFFER_LOCALWORLDMATRIX (スキニング用ボーン行列)
//   b4 : ユーザー定数バッファ (Component::CUSTOM_SHADER_CB_SLOT)
// 行列は float4 の行配列として格納されているため、dot() で各成分を求める。

struct DX_D3D11_VS_CONST_BUFFER_BASE
{
    float4 AntiViewportMatrix[4];
    float4 ProjectionMatrix[4];
    float4 ViewMatrix[3];
    float4 LocalWorldMatrix[3];
    float4 ToonOutLineSize;
    float  DiffuseSource;
    float  SpecularSource;
    float  MulSpecularColor;
    float  Padding;
};

cbuffer cbD3D11_CONST_BUFFER_VS_BASE : register(b1)
{
    DX_D3D11_VS_CONST_BUFFER_BASE g_Base;
};

// TreeLeafSway::TreeWindCB と同じ並び
cbuffer TreeWindBuffer : register(b4)
{
    float4 wind;           // x=time y=振幅 z=speed w=frequency
    float4 windDirection;  // xz=向き(正規化済み)
    float4 lightDirection;
    float4 lightColor;     // w=ambient
};

// MV1 モデルの頂点入力 (剛体メッシュ)。ProximityReveal_VS.hlsl と同じレイアウト。
// TexCoords1 には Blender でベイクした揺れマスクが入っている
// (x=揺れの重み 0..1 / 樹皮は0、y=葉カードごとのランダム位相 0..1)。
struct VS_INPUT
{
    float3 Position   : POSITION;
    float3 Normal     : NORMAL0;
    float4 Diffuse    : COLOR0;
    float4 Specular   : COLOR1;
    float4 TexCoords0 : TEXCOORD0;
    float4 TexCoords1 : TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal   : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float weight  = input.TexCoords1.x;
    float phase01 = input.TexCoords1.y;

    // ローカル座標 → ワールド座標
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos;
    worldPos.x = dot(localPos, g_Base.LocalWorldMatrix[0]);
    worldPos.y = dot(localPos, g_Base.LocalWorldMatrix[1]);
    worldPos.z = dot(localPos, g_Base.LocalWorldMatrix[2]);
    worldPos.w = 1.0f;

    // 位相にワールド座標を混ぜて、風が林を波として走るようにする
    float2 direction = windDirection.xz;
    float  phase     = dot(worldPos.xz, direction) * wind.w + wind.x * wind.z + phase01 * 6.2831853f;
    float  sway      = wind.y * (0.6f + 0.4f * sin(phase)) + 0.25f * wind.y * sin(phase * 2.3f + 1.7f);
    worldPos.xz += direction * sway * weight;

    // ワールド座標 → ビュー座標
    float4 viewPos;
    viewPos.x = dot(worldPos, g_Base.ViewMatrix[0]);
    viewPos.y = dot(worldPos, g_Base.ViewMatrix[1]);
    viewPos.z = dot(worldPos, g_Base.ViewMatrix[2]);
    viewPos.w = 1.0f;

    // ビュー座標 → 射影座標
    output.Position.x = dot(viewPos, g_Base.ProjectionMatrix[0]);
    output.Position.y = dot(viewPos, g_Base.ProjectionMatrix[1]);
    output.Position.z = dot(viewPos, g_Base.ProjectionMatrix[2]);
    output.Position.w = dot(viewPos, g_Base.ProjectionMatrix[3]);

    output.Normal   = input.Normal;
    output.TexCoord = input.TexCoords0.xy;
    return output;
}
