#include "Engine_Physics_PhysicsLayer.h"

#include <algorithm>
#include <array>
#include <atomic>

#include "ImGuiHelper.h"
#include "../../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Module::Physics
{
    namespace
    {
        constexpr auto DEFAULT_LAYER_NAME = "Default";
    }

    struct PhysicsLayers::Table
    {
        std::array<std::string, MAX_LAYER_COUNT> names{ DEFAULT_LAYER_NAME };
        std::array<const char*, MAX_LAYER_COUNT> namePointers{};
        int count = 1;
        // WARNING: Jolt のワーカースレッドから読まれる
        std::array<std::atomic<LayerMask>, MAX_LAYER_COUNT> collisionMasks;

        Table()
        {
            for (auto& mask : collisionMasks)
                mask.store(ALL_LAYERS_MASK, std::memory_order_relaxed);
            RefreshNamePointers();
        }

        void RefreshNamePointers()
        {
            for (int i = 0; i < MAX_LAYER_COUNT; ++i)
                namePointers[i] = names[i].c_str();
        }
    };

    PhysicsLayers::Table& PhysicsLayers::GetTable()
    {
        static Table table;
        return table;
    }

    bool PhysicsLayers::IsValidIndex(const int index)
    {
        return index >= 0 && index < GetTable().count;
    }

    int PhysicsLayers::Count()
    {
        return GetTable().count;
    }

    const char* const* PhysicsLayers::Names()
    {
        return GetTable().namePointers.data();
    }

    Layer PhysicsLayers::ToLayer(const int index)
    {
        return IsValidIndex(index) ? static_cast<Layer>(index) : Layer::Default;
    }

    const char* PhysicsLayers::ToName(const Layer layer)
    {
        const int index = ToIndex(layer);
        return IsValidIndex(index) ? GetTable().namePointers[index] : "(Invalid)";
    }

    Layer PhysicsLayers::NameToLayer(const std::string_view name)
    {
        const auto& table = GetTable();
        for (int i = 0; i < table.count; ++i)
        {
            if (table.names[i] == name)
                return static_cast<Layer>(i);
        }
        LogWarning("Physics: レイヤー \"" + std::string(name) + "\" は ProjectConfig/Physics/LayerNames.json に定義されていません -> Default を使用します");
        return Layer::Default;
    }

    void PhysicsLayers::SetNames(const std::vector<std::string>& names)
    {
        auto& table = GetTable();
        table.count = std::clamp(static_cast<int>(names.size()), 1, MAX_LAYER_COUNT);
        for (int i = 0; i < MAX_LAYER_COUNT; ++i)
            table.names[i] = i < table.count && i < static_cast<int>(names.size()) ? names[i] : std::string();
        if (table.names[0].empty())
            table.names[0] = DEFAULT_LAYER_NAME;
        table.RefreshNamePointers();
    }

    LayerMask PhysicsLayers::CollisionMaskOf(const Layer layer)
    {
        const int index = ToIndex(layer);
        if (index < 0 || index >= MAX_LAYER_COUNT)
            return 0;
        return GetTable().collisionMasks[index].load(std::memory_order_relaxed);
    }

    void PhysicsLayers::SetCollisionMaskOf(const Layer layer, const LayerMask mask)
    {
        const int index = ToIndex(layer);
        if (index < 0 || index >= MAX_LAYER_COUNT)
            return;
        GetTable().collisionMasks[index].store(mask, std::memory_order_relaxed);
    }

    void PhysicsLayers::SetLayersCollide(const Layer a, const Layer b, const bool collide)
    {
        auto update = [collide](const Layer self, const Layer other)
        {
            LayerMask mask = CollisionMaskOf(self);
            if (collide)
                AddLayer(mask, other);
            else
                RemoveLayer(mask, other);
            SetCollisionMaskOf(self, mask);
        };
        update(a, b);
        update(b, a);
    }

    bool PhysicsLayers::LayersCollide(const Layer a, const Layer b)
    {
        // NOTE: 非対称なマスクでも片側が拒否すれば当たらない
        return HasLayer(CollisionMaskOf(a), b) && HasLayer(CollisionMaskOf(b), a);
    }

    bool PhysicsLayers::DrawChoiceGui(const char* label, Layer& layer)
    {
        int current = ToIndex(layer);
        bool changed = false;

        if (ImGui::Combo(label, &current, Names(), Count()))
        {
            layer = ToLayer(current);
            changed = true;
        }

        return changed;
    }

    bool PhysicsLayers::DrawMaskGui(const char* label, LayerMask& mask)
    {
        bool changed = false;

        ImGui::TextUnformatted(label);
        ImGui::PushID(label);
        for (int i = 0; i < Count(); ++i)
        {
            const Layer layer = ToLayer(i);

            bool enabled = HasLayer(mask, layer);
            if (ImGui::Checkbox(ToName(layer), &enabled))
            {
                if (enabled)
                    AddLayer(mask, layer);
                else
                    RemoveLayer(mask, layer);

                changed = true;
            }
        }
        ImGui::PopID();

        return changed;
    }

    bool PhysicsLayers::DrawCollisionMatrixGui()
    {
        bool changed = false;
        const int count = Count();

        ImGui::PushID("LayerCollisionMatrix");
        for (int row = 0; row < count; ++row)
        {
            const Layer rowLayer = ToLayer(row);
            ImGui::TextUnformatted(ToName(rowLayer));
            for (int column = row; column < count; ++column)
            {
                const Layer columnLayer = ToLayer(column);
                bool collide = LayersCollide(rowLayer, columnLayer);

                ImGui::SameLine();
                ImGui::PushID(row * MAX_LAYER_COUNT + column);
                if (ImGui::Checkbox("##collide", &collide))
                {
                    SetLayersCollide(rowLayer, columnLayer, collide);
                    changed = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s x %s", ToName(rowLayer), ToName(columnLayer));
                ImGui::PopID();
            }
        }
        ImGui::PopID();

        return changed;
    }
}
