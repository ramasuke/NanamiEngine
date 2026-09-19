#pragma once
#include "../../../../../../Engine/Module/Component/ComponentBase.h"

namespace GameCore::PlayerAvatar
{
    // ロックオン対象の子孫に付けて EnemyBase::lockOnPoint_ から指すと、その位置がロックオン位置になる
    class LockOnPoint final : public Component::ComponentBase
    {
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GameCore::PlayerAvatar::LockOnPoint, 0)
