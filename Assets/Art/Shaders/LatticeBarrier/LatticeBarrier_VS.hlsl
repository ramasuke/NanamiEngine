// LatticeBarrier 用 頂点シェーダー (DxLib Direct3D 11 / QuadRendererの板ポリゴン用)
//
// DxLib が頂点シェーダーへ渡す定数バッファ (DxShader_VS_D3D11.h / VertexShader.h):
//   b0 : DX_D3D11_CONST_BUFFER_COMMON              (ライト・マテリアル・フォグ)
//   b1 : DX_D3D11_VS_CONST_BUFFER_BASE             (射影・ビュー・ローカル→ワールド行列)
//   b2 : DX_D3D11_VS_CONST_BUFFER_OTHERMATRIX      (シャドウマップ行列・テクスチャ行列)
//   b3 : DX_D3D11_VS_CONST_BUFFER_LOCALWORLDMATRIX (スキニング用ボーン行列)
//   b4 : ユーザー定数バッファ (QuadRenderer::CUSTOM_SHADER_CB_SLOT)
// 行列は float4 の行配列として格納されているため、dot() で各成分を求める。
// (ProximityReveal_VS.hlsl と同じ変換手順。MV1DrawModelではなくQuadRendererの
//  DrawPrimitive3DToShader2(VERTEX3D配列)経由で描画される点のみが異なる。
//  ワールド行列はMV1SetMatrixの代わりにSetTransformToWorldで設定される)

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

// VERTEX3D (DxLib.h) の頂点入力。su/svはDxLib内部の補助UVのため未使用。
struct VS_INPUT
{
    float3 Position   : POSITION;
    float3 Normal     : NORMAL0;
    float4 Diffuse    : COLOR0;
    float4 Specular   : COLOR1;
    float4 TexCoords0 : TEXCOORD0; // xy = u,v (板ポリゴン上のローカルUV 0..1)
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 localPos = float4(input.Position, 1.0f);

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

    output.WorldPos = worldPos.xyz;
    output.TexCoord = input.TexCoords0.xy;
    return output;
}
