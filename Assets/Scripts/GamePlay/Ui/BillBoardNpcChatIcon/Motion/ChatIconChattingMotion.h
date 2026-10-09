#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "ChatIconPopMotion.h"

namespace GamePlay::Ui
{
    // NOTE: 会話中の NPC の目印。呼吸するように拡大縮小する
    class ChatIconChattingMotion final : public Component::ComponentBase,
                                         public LifeCycleCallback::IUpdatable
    {
    private:
        void OnUpdate() override;

        ChatIconPopMotion pop_;

        [[serialize(0)]] float popDuration_secs_  = 0.25f;
        [[serialize(0)]] float breathScale_       = 0.05f;
        [[serialize(0)]] float breathPeriod_secs_ = 1.6f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(popDuration_secs_));
            archive(CEREAL_NVP(breathScale_));
            archive(CEREAL_NVP(breathPeriod_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(popDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(breathScale_));
            if (version >= 0) archive(CEREAL_NVP(breathPeriod_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ChatIconChattingMotion, 0);
