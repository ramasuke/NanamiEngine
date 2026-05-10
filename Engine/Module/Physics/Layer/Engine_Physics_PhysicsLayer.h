#pragma once

namespace NanamiEngine::Module::Physics
{
    enum class Layer : int
    {
        Default = 0,
        Count
    };
    
    static constexpr const char* LAYER_NAMES[] = {
        "Default",
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
}
