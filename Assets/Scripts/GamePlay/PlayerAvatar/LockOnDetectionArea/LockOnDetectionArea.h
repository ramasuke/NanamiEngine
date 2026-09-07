#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "../../../../../Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"

namespace GameCore::PlayerAvatar
{
    class ILockOnTarget;
}

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    class LockOnDetectionArea final : public Component::ComponentBase,
                                      public Physics::Callback::ISensorEnterable,
                                      public Physics::Callback::ISensorExitable
    {
    public:
        [[nodiscard]] const std::vector<std::weak_ptr<GameObject::IGameObject>>& Candidates() const;

    private:
        void OnTriggerEnter(const Physics::Manifold& contactManifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        std::vector<std::weak_ptr<GameObject::IGameObject>> candidates_;

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

ENGINE_REGISTER_COMPONENT(GamePlay::PlayerAvatar::LockOnDetectionArea, 0)
