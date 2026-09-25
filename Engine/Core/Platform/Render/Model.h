#pragma once
#include <string>

#include "mat4x4.hpp"

// 読み込み済みモデル (MV1 ハンドル) への問い合わせ。ModelRenderer::modelDxLibHandle_ などの int ハンドルを渡す
namespace NanamiEngine::Platform::Render::Model
{
    /** @brief 名前のフレーム (ボーン) 番号。無ければ -1 */
    [[nodiscard]] int       SearchFrame(int modelHandle, const std::string& utf8FrameName);
    /** @brief 最後に MV1SetMatrix された描画行列 */
    [[nodiscard]] glm::mat4 GetMatrix(int modelHandle);
    /** @brief フレームのローカル -> ワールド行列 (GetMatrix 基準) */
    [[nodiscard]] glm::mat4 GetFrameLocalWorldMatrix(int modelHandle, int frameIndex);
}
