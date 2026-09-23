#pragma once


namespace LibCore::Dxlib
{
    enum class BlendMode : int
    {
        NoBlend = 0,
        Alpha   = 1,
        Add     = 2,
        Sub     = 3,
        Mul     = 4,
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
