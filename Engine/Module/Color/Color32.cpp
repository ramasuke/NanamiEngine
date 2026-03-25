#include "Color32.h"
#include "DxLib.h"

NanamiEngine::Color32::Color32(const uint8_t r, const uint8_t g, const uint8_t b)
    : r_(r), g_(g), b_(b)
{
}

int NanamiEngine::Color32::ToDxColor() const
{
    return GetColor(r_, g_, b_);
}