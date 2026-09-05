// ProximityReveal 用 頂点シェーダー (剛体メッシュ用 / DxLib Direct3D 11)
//
// DxLib が頂点シェーダーへ渡す定数バッファ (DxShader_VS_D3D11.h / VertexShader.h):
//   b0 : DX_D3D11_CONST_BUFFER_COMMON              (ライト・マテリアル・フォグ)
//   b1 : DX_D3D11_VS_CONST_BUFFER_BASE             (射影・ビュー・ローカル→ワールド行列)
//   b2 : DX_D3D11_VS_CONST_BUFFER_OTHERMATRIX      (シャドウマップ行列・テクスチャ行列)
//   b3 : DX_D3D11_VS_CONST_BUFFER_LOCALWORLDMATRIX (スキニング用ボーン行列)
//   b4 : ユーザー定数バッファ (ModelRenderer::CUSTOM_SHADER_CB_SLOT)
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

// MV1 モデルの頂点入力 (剛体メッシュ)。
// ここに列挙した要素は DxLib の全頂点タイプのレイアウトに含まれるので、
// 法線マップ付き (TANGENT/BINORMAL 追加) や 9 ボーン以上 (FREE_FRAME, CPU スキニング済み) の
// トライアングルリストでもそのまま使える。
// 4/8 ボーンのスキンメッシュ (BLENDINDICES/BLENDWEIGHT) はスキニングが必要なので対象外。
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
    float3 WorldPos : TEXCOORD0; // ピクセルシェーダーへのワールド座標
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
