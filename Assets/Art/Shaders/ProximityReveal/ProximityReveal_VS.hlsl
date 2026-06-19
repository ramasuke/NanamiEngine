// DxLib が b0 に自動でセットするマトリクス定数バッファ
cbuffer DxLib_VSConstantBuffer : register(b0)
{
    float4x4 mWorld;
    float4x4 mView;
    float4x4 mProjection;
    float4x4 mWorldViewProj;
};

// MV1モデル (VERTEX3DSHADER) の頂点入力レイアウト
struct VS_INPUT
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float4 Diffuse   : COLOR0;
    float4 Specular  : COLOR1;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
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
    float4 worldPos  = mul(float4(input.Position, 1.0f), mWorld);
    output.Position  = mul(float4(input.Position, 1.0f), mWorldViewProj);
    output.WorldPos  = worldPos.xyz;
    output.TexCoord  = input.TexCoord0;
    return output;
}
