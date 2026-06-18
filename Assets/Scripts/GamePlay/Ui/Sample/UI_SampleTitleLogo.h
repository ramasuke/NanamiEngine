#pragma once
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    class SampleTitleLogo final : public Component::ComponentBase,
                                  public LifeCycleCallback::IUpdatable
    {
        void OnUpdate() override;

        [[serialize(0)]] float titleLogoDuration_secs_ = 0.0f;
        [[serialize(0)]] float titleLogoDuring_secs_   = 0.0f;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(titleLogoDuration_secs_));
            archive(CEREAL_NVP(titleLogoDuring_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(titleLogoDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(titleLogoDuring_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SampleTitleLogo, 0)
