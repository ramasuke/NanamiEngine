#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Engine/Module/Physics/ContactCallback/SensorExitable/Engine_Physics_ISensorExitable.h"
#include "../../Engine/Module/Component/ComponentBase.h"
#include "../R4.h"

namespace NanamiEngine::R4
{
    class NANAMI_API SensorExitableAsObservable final : public Component::ComponentBase,
                                             public Physics::Callback::ISensorExitable
    {
    public:
        [[nodiscard]] Observable<std::shared_ptr<GameObject::IGameObject>> OnAction() const;

    private:
        void OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject) override;

        Subject<std::shared_ptr<GameObject::IGameObject>> onAction_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorExitable>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<Physics::Callback::ISensorExitable>(this));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::R4::SensorExitableAsObservable, 0);
