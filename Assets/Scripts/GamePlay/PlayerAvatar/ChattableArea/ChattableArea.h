#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorStayable/Engine_Physics_ISensorStayable.h"

namespace GameCore::PlayerAvatar
{
    class IPlayerChattable;
}

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    class ChattableArea final : public Component::ComponentBase,
                                public Physics::Callback::ISensorEnterable,
                                public Physics::Callback::ISensorExitable
    {
    public:
        [[nodiscard]] std::weak_ptr<IPlayerChattable> CatchChatTarget();

    private:
        void OnTriggerEnter(const Physics::Manifold& contactManifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        
        std::vector<std::weak_ptr<IPlayerChattable>> playerChattableTargets_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<ISensorEnterable>(this));
            archive(cereal::base_class<ISensorExitable>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<ISensorEnterable>(this));
            archive(cereal::base_class<ISensorExitable>(this));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::ChattableArea, 0);
CEREAL_REGISTER_TYPE(GamePlay::PlayerAvatar::ChattableArea);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::PlayerAvatar::ChattableArea);
#pragma endregion
