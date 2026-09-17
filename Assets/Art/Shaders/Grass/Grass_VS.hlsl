// Grass 用 頂点シェーダー (DxLib Direct3D 11 / GrassRenderer の頂点バッファ用)
//
// DxLib が頂点シェーダーへ渡す定数バッファ (DxShader_VS_D3D11.h / VertexShader.h):
//   b0 : DX_D3D11_CONST_BUFFER_COMMON              (ライト・マテリアル・フォグ)
//   b1 : DX_D3D11_VS_CONST_BUFFER_BASE             (射影・ビュー・ローカル→ワールド行列)
//   b2 : DX_D3D11_VS_CONST_BUFFER_OTHERMATRIX      (シャドウマップ行列・テクスチャ行列)
//   b3 : DX_D3D11_VS_CONST_BUFFER_LOCALWORLDMATRIX (スキニング用ボーン行列)
//   b4 : ユーザー定数バッファ (GrassRenderer::CUSTOM_SHADER_CB_SLOT)
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

// GrassRenderer::GrassCB と同じ並び
cbuffer GrassBuffer : register(b4)
{
    float4 wind;           // x=time y=strength z=speed w=frequency
    float4 windDirection;  // xz=向き(正規化済み)
    float4 baseColor;
    float4 tipColor;
    float4 lightDirection;
    float4 lightColor;     // w=ambient
};

// VERTEX3DSHADER (DxLib.h) の頂点入力。
// SubPosition.xyz=葉の根元のワールド座標 w=揺れの位相(0..1)
// TexCoords0.x=根元からの高さ比率(0..1) y=葉の高さ
struct VS_INPUT
{
    float3 Position    : POSITION0;
    float4 SubPosition : POSITION1;
    float3 Normal      : NORMAL0;
    float3 Tangent     : TANGENT0;
    float3 Binormal    : BINORMAL0;
    float4 Diffuse     : COLOR0;
    float4 Specular    : COLOR1;
    float2 TexCoords0  : TEXCOORD0;
    float2 TexCoords1  : TEXCOORD1;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR0;
    float3 Normal   : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float heightRatio = input.TexCoords0.x;
    float bladeHeight = max(input.TexCoords0.y, 1.0f);
    float weight      = heightRatio * heightRatio;

    float2 direction = windDirection.xz;
    float  phase     = dot(input.SubPosition.xz, direction) * wind.w + wind.x * wind.z + input.SubPosition.w * 6.2831853f;
    float  sway      = wind.y * (0.6f + 0.4f * sin(phase)) + 0.25f * wind.y * sin(phase * 2.3f + 1.7f);

    float3 position = input.Position;
    position.xz += direction * sway * weight;
    // 先端が横へ動いた分だけ下げて、葉が伸びて見えないようにする
    position.y  -= min(0.5f * sway * sway * weight / bladeHeight, bladeHeight * heightRatio * 0.9f);

    float4 localPos = float4(position, 1.0f);

    // ローカル座標 → ワールド座標
    float4 worldPos;
    worldPos.x = dot(localPos, g_Base.LocalWorldMatrix[0]);
    worldPos.y = dot(localPos, g_Base.LocalWorldMatrix[1]);
    worldPos.z = dot(localPos, g_Base.LocalWorldMatrix[2]);
    worldPos.w = 1.0f;

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

    output.Color  = lerp(baseColor.rgb, tipColor.rgb, heightRatio) * input.Diffuse.rgb;
    output.Normal = input.Normal;
    return output;
}
