#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/ModelRenderer/ModelRenderer.h"

namespace GamePlay::Prop
{
    class ProximityReveal final : public Component::ComponentBase,
                                  public LifeCycleCallback::IAwakable,
                                  public LifeCycleCallback::IUpdatable
    {
    private:
        struct ProximityCB
        {
            float playerPos[3];
            float revealRadius;
            float transitionWidth;
            float pad[3];
        };

        void OnAwake () override;
        void OnUpdate() override;

        float revealRadius_    = 5.0f;
        float transitionWidth_ = 3.0f;

        std::weak_ptr<Component::ModelRenderer> modelRenderer_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(revealRadius_));
            archive(CEREAL_NVP(transitionWidth_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(revealRadius_));
            if (version >= 0) archive(CEREAL_NVP(transitionWidth_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ProximityReveal, 0)
