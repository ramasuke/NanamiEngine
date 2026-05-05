#pragma once
#include "vec3.hpp"
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
        Mode mode_ = Mode::EnemyOffset;
        FIELD(GameObject::IGameObject) targetObject_;

    public:
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(mode_));
            archive(CEREAL_NVP(targetObject_));
        }
    };
}
