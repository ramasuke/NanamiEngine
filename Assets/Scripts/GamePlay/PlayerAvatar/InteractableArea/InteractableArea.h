#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Physics/ContactCallback/SensorEnterable/Engine_Physics_ISensorEnterable.h"
#include "Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "Engine/Module/Physics/ContactCallback/SensorStayable/Engine_Physics_ISensorStayable.h"

namespace GameCore::PlayerAvatar
{
    class IPlayerInteractable;
}

namespace GamePlay::PlayerAvatar
{
    using namespace GameCore::PlayerAvatar;
    class InteractableArea final : public Component::ComponentBase,
                                public Physics::Callback::ISensorEnterable,
                                public Physics::Callback::ISensorExitable
    {
    public:
        [[nodiscard]] std::weak_ptr<IPlayerInteractable> CatchInteractTarget();
        /**
         * @brief OnInteractable を呼んでいる最中に、それが自分の (ローカルの) アバターから来たかどうか
         * NOTE: 他のプレイヤーが近づいただけで UI の音を鳴らさないために使う
         */
        [[nodiscard]] static bool IsNotifyingOwner() { return isNotifyingOwner_; }

    private:
        void OnTriggerEnter(const Physics::Manifold& contactManifold, const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        void OnTriggerExit (const std::shared_ptr<GameObject::IGameObject>& gameObject) override;
        
        std::vector<std::weak_ptr<IPlayerInteractable>> playerInteractableTargets_;
        static inline bool isNotifyingOwner_ = false;

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

CEREAL_CLASS_VERSION(GamePlay::PlayerAvatar::InteractableArea, 0);
