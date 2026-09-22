#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Sound
{
    // 生成されてから delay_secs_ 後に、この位置で SE を1回鳴らす。着弾や詠唱など、エフェクトのプレハブに音を持たせる用
    class SpawnSound final : public Component::ComponentBase,
                             public LifeCycleCallback::IUpdatable
    {
        void OnUpdate() override;

        [[serialize(0)]] FIELD(Asset::SoundFile) sound_;
        [[serialize(0)]] float delay_secs_ = 0.0f;

        float elapsed_secs_ = 0.0f;
        bool  hasPlayed_    = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(sound_));
            archive(CEREAL_NVP(delay_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(sound_));
            if (version >= 0) archive(CEREAL_NVP(delay_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Sound::SpawnSound, 0)
