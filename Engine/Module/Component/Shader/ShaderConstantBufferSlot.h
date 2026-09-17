#pragma once

namespace NanamiEngine::Module::Component
{
    // DxLib(Direct3D 11) は定数バッファスロット b0～b3 を内部で使用しているため、
    // カスタムシェーダー用の定数バッファは b4 に割り当てる。
    constexpr int CUSTOM_SHADER_CB_SLOT = 4;
    constexpr int CUSTOM_SHADER_CB_SIZE = 256;
}
