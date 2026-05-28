#pragma once
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    enum class Layer : uint32_t
    {
        Default = 0,
        Player,
        Count
    };

    using LayerMask = uint32_t;
    constexpr LayerMask CreateLayerMask()
    {
        return 0;
    }

    constexpr LayerMask ToMask(Layer layer)
    {
        return 1u << static_cast<uint32_t>(layer);
    }
    
    static constexpr const char* LAYER_NAMES[] = {
        "Default",
        "Player"
    };

    static_assert(
        static_cast<int>(Layer::Count) == sizeof(LAYER_NAMES) / sizeof(const char*),
        "LayerNames count must match Layer enum count!"
    );
    
    [[nodiscard]] constexpr Layer ToLayer(int index)
    {
        if (index < 0 || index >= static_cast<int>(Layer::Count))
        {
            return Layer::Default;
        }
        return static_cast<Layer>(index);
    }
    
    [[nodiscard]] constexpr int ToIndex(const Layer layer)
    {
        return static_cast<int>(layer);
    }
    
    [[nodiscard]] constexpr const char* ToName(const Layer layer)
    {
        return LAYER_NAMES[ToIndex(layer)];
    }

    // 現在のLayerをGUIで選択
    // 戻り値：変更されたかどうか
    bool DrawChoiceLayerGui(const char* label, Layer& layer);
    
    constexpr bool HasLayer(const LayerMask mask, const Layer layer)
    {
        return mask & ToMask(layer);
    }

    constexpr void AddLayer(LayerMask& mask, const Layer layer)
    {
        mask |= ToMask(layer);
    }

    constexpr void RemoveLayer(LayerMask& mask, const Layer layer)
    {
        mask &= ~ToMask(layer);
    }
}
