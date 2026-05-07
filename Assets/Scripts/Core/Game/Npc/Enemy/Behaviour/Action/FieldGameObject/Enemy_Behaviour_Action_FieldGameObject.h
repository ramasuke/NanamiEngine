#pragma once
#include "../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../TickContext/Enemy_Behaviour_TickContext.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    struct TickContext;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    template<typename T>
    class FieldGameObject final
    {
    public:
        enum class Mode
        {
            OwnObject   ,
            PrefabObject,
            SceneObject ,
        };
        
        [[nodiscard]] std::shared_ptr<T> get(const TickContext& context) const
        {
            switch (mode_)
            {
            case Mode::OwnObject:
                if constexpr (std::is_base_of_v<GameObject::IGameObject, T>)
                {
                    return context.EnemyTransform().GetGameObject();
                }
                else if constexpr (std::is_base_of_v<Component::ComponentBase, T>)
                {
                    return context.EnemyGameObject().Components().Catch<T>().lock();
                }
            case Mode::PrefabObject:
                if constexpr (std::is_base_of_v<GameObject::IGameObject, T>)
                {
                    return context.EnemyTransform().CatchChild(prefabObjectName_);
                }
                else if constexpr (std::is_base_of_v<Component::ComponentBase, T>)
                {
                    return context.EnemyTransform().CatchChild(prefabObjectName_)->Components().Catch<T>().lock();
                }
                break;
            case Mode::SceneObject:
                return sceneGameObject_.get();
            }
            return nullptr;
        }

    private:
        [[serialize(0)]] Mode mode_ = Mode::PrefabObject;
        [[serialize(0)]] FIELD(T) sceneGameObject_;
        [[serialize(0)]] std::string prefabObjectName_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            const char* items[] = {
                "OwnObject",
                "PrefabObject",
                "SceneObject"
            };
            int current = static_cast<int>(mode_);
            if (ImGui::Combo("mode_", &current, items, IM_ARRAYSIZE(items)))
            {
                mode_ = static_cast<Mode>(current);
            }
            switch (mode_)
            {
            case Mode::OwnObject:
                break;
            case Mode::PrefabObject:
                ImGuiHelper::OnDrawInputField("prefabObjectName_", prefabObjectName_);
                break;
            case Mode::SceneObject:
                ImGuiHelper::OnDrawInputField("sceneGameObject_", sceneGameObject_);
                break;
            default:
                break;
            }
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(mode_));
            archive(CEREAL_NVP(sceneGameObject_));
            archive(CEREAL_NVP(prefabObjectName_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 0) archive(CEREAL_NVP(mode_));
            if (version >= 0) archive(CEREAL_NVP(sceneGameObject_));
            if (version >= 0) archive(CEREAL_NVP(prefabObjectName_));
        }
#pragma endregion
    };
}
