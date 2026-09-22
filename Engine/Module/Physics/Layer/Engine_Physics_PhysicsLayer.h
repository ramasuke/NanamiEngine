#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NanamiEngine::Module::Physics
{
    // NOTE: Default 以外のレイヤーは ProjectConfig で定義する
    enum class Layer : uint32_t
    {
        Default = 0,
    };

    // NOTE: LayerMask のビット数が上限
    constexpr int MAX_LAYER_COUNT = 32;

    using LayerMask = uint32_t;
    constexpr LayerMask ALL_LAYERS_MASK = ~LayerMask(0);

    constexpr LayerMask CreateLayerMask()
    {
        return 0;
    }

    constexpr LayerMask ToMask(Layer layer)
    {
        return 1u << static_cast<uint32_t>(layer);
    }

    [[nodiscard]] constexpr int ToIndex(const Layer layer)
    {
        return static_cast<int>(layer);
    }

    [[nodiscard]] int LayerCount();
    // WARNING: 次の SetLayerNames で無効になる
    [[nodiscard]] const char* const* LayerNames();

    // NOTE: 範囲外は Default
    [[nodiscard]] Layer ToLayer(int index);
    [[nodiscard]] const char* ToName(Layer layer);
    // NOTE: 見つからなければ Default
    [[nodiscard]] Layer NameToLayer(std::string_view name);

    // NOTE: 先頭は常に Default
    void SetLayerNames(const std::vector<std::string>& names);

    [[nodiscard]] LayerMask CollisionMaskOf(Layer layer);
    void SetCollisionMaskOf(Layer layer, LayerMask mask);
    void SetLayersCollide(Layer a, Layer b, bool collide);
    [[nodiscard]] bool LayersCollide(Layer a, Layer b);

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

    // 戻り値：変更されたかどうか
    bool DrawCollisionMatrixGui();
}
