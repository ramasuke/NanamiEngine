#pragma once
#include <cstdint>

namespace NanamiEngine::Module::Physics
{
    enum class Layer : uint32_t
    {
        Default = 0,
        Player,
        Enemy,
        Boundary,
        BodyPart,
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
        "Player",
        "Enemy",
        "Boundary",
        "BodyPart"
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

    // LayerMask をチェックボックスで編集
    // 戻り値：変更されたかどうか
    bool DrawLayerMaskGui(const char* label, LayerMask& mask);

    // Boundary は Player / Enemy だけを止める
    [[nodiscard]] constexpr bool LayersCollide(const Layer a, const Layer b)
    {
        if (a == Layer::Boundary) return b == Layer::Player || b == Layer::Enemy;
        if (b == Layer::Boundary) return a == Layer::Player || a == Layer::Enemy;
        return true;
    }
}
