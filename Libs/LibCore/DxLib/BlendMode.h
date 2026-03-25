#pragma once
#include "DxLib.h"

namespace LibCore::Dxlib
{
    enum class BlendMode : int
    {
        NoBlend = DX_BLENDMODE_NOBLEND,
        Alpha   = DX_BLENDMODE_ALPHA,
        Add     = DX_BLENDMODE_ADD,
        Sub     = DX_BLENDMODE_SUB,
        Mul     = DX_BLENDMODE_MUL,
    };
    
    inline static const char* BlendModeLabelNames[] =
    {
        "NoBlend",
        "Alpha",
        "Add",
        "Sub",
        "Mul"
    };
}
