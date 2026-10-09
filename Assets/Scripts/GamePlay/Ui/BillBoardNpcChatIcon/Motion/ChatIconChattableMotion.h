#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "ChatIconPopMotion.h"

namespace GamePlay::Ui
{
    // NOTE: 話しかけられる NPC の目印。下向きに弾む
    class ChatIconChattableMotion final : public Component::ComponentBase,
                                          public LifeCycleCallback::IUpdatable
    {
    private:
        void OnUpdate() override;

        ChatIconPopMotion pop_;

        [[serialize(0)]] float popDuration_secs_ = 0.25f;
        [[serialize(0)]] float bounceAmplitude_  = 0.12f;
        [[serialize(0)]] float bounceSpeed_      = 4.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(popDuration_secs_));
            archive(CEREAL_NVP(bounceAmplitude_));
            archive(CEREAL_NVP(bounceSpeed_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(popDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(bounceAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(bounceSpeed_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ChatIconChattableMotion, 0);
