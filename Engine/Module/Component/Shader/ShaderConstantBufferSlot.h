#pragma once

namespace NanamiEngine::Module::Component
{
    // NOTE: カスタムシェーダーの定数バッファは b4 (b0〜b3 は DxLib が使う)
    constexpr int CUSTOM_SHADER_CB_SLOT = 4;
    constexpr int CUSTOM_SHADER_CB_SIZE = 256;
}
