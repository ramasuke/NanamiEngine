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

    /** @brief ProjectConfig/Physics で定義されたレイヤー名と衝突マトリクス */
    class PhysicsLayers final
    {
    public:
        PhysicsLayers() = delete;

        [[nodiscard]] static int Count();
        // WARNING: 次の SetNames で無効になる
        [[nodiscard]] static const char* const* Names();

        // NOTE: 範囲外は Default
        [[nodiscard]] static Layer ToLayer(int index);
        [[nodiscard]] static const char* ToName(Layer layer);
        // NOTE: 見つからなければ Default
        [[nodiscard]] static Layer NameToLayer(std::string_view name);

        // NOTE: 先頭は常に Default
        static void SetNames(const std::vector<std::string>& names);

        [[nodiscard]] static LayerMask CollisionMaskOf(Layer layer);
        static void SetCollisionMaskOf(Layer layer, LayerMask mask);
        static void SetLayersCollide(Layer a, Layer b, bool collide);
        [[nodiscard]] static bool LayersCollide(Layer a, Layer b);

        // 現在のLayerをGUIで選択
        // 戻り値：変更されたかどうか
        static bool DrawChoiceGui(const char* label, Layer& layer);

        // LayerMask をチェックボックスで編集
        // 戻り値：変更されたかどうか
        static bool DrawMaskGui(const char* label, LayerMask& mask);

        // 戻り値：変更されたかどうか
        static bool DrawCollisionMatrixGui();

    private:
        struct Table;

        // NOTE: 静的初期化中 (既定値の NameToLayer 等) にも使えるよう関数内 static で持つ
        [[nodiscard]] static Table& GetTable();
        [[nodiscard]] static bool IsValidIndex(int index);
    };
}
