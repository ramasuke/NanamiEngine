#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "mat4x4.hpp"

// NOTE: 読み込み済みモデル (MV1 の int ハンドル) への問い合わせ
namespace NanamiEngine::Platform::Render::Model
{
    // NOTE: 名前のフレーム (ボーン) 番号。無ければ -1
    [[nodiscard]] NANAMI_API int       SearchFrame(int modelHandle, const std::string& utf8FrameName);
    // NOTE: 最後に MV1SetMatrix された描画行列
    [[nodiscard]] NANAMI_API glm::mat4 GetMatrix(int modelHandle);
    // NOTE: フレームのローカル -> ワールド行列 (GetMatrix 基準)
    [[nodiscard]] NANAMI_API glm::mat4 GetFrameLocalWorldMatrix(int modelHandle, int frameIndex);
}
