#pragma once
#include <vector>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"

namespace GameCore::PlayerAvatar
{
    class IPlayerPickable;
}

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    /**
     * @brief アバターの子に置く拾い範囲。センサーに入った IPlayerPickable を、拾えるようになった時点で拾う
     */
    class PickupArea final : public Component::ComponentBase,
                             public LifeCycleCallback::IUpdatable,
                             public Physics::Callback::ISensorEnterable,
                             public Physics::Callback::ISensorExitable
    {
    private:
        void OnUpdate() override;
        void OnTriggerEnter(const Physics::Manifold& contactManifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        std::vector<std::weak_ptr<IPlayerPickable>> pickables_;

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

CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::PickupArea, 0);
