#pragma once
#include "../glm/vec3.hpp"
#include "../../../../../../../../../Engine/Core/Object/Field/Field.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    struct TickContext;
}

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class Position final
    {
    public:
        enum class Mode
        {
            EnemyOffset,
            AbsolutePosition,
            TargetObject
        };

        [[nodiscard]] glm::vec3 get(const TickContext& context) const;
        void OnDrawGui();

    private:
        glm::vec3 offset_ = {};
        glm::vec3 offsetRotation_ = {};
        Mode mode_ = Mode::EnemyOffset;
        FIELD(GameObject::IGameObject) targetObject_;

    public:
        template<class Archive>
        void save(Archive& archive, size_t version) const
        {
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(offsetRotation_));
            archive(CEREAL_NVP(mode_));
            archive(CEREAL_NVP(targetObject_));
        }

        template<class Archive>
        void load(Archive& archive, size_t version)
        {
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(offsetRotation_));
            archive(CEREAL_NVP(mode_));
            archive(CEREAL_NVP(targetObject_));
        }
        
        // template<class Archive>
        // void serialize(Archive& archive)
        // {
        //     archive(CEREAL_NVP(offset_));
        //     // archive(CEREAL_NVP(offsetRotation_));
        //     archive(CEREAL_NVP(mode_));
        //     archive(CEREAL_NVP(targetObject_));
        // }
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::Position, 1)