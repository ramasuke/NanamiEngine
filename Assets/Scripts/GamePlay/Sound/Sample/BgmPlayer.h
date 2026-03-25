#pragma once
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Sound
{
    class BgmPlayObject final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable,
                                public LifeCycleCallback::IStartable
    {
        void OnAwake() override;
        void OnStart() override;

        [[serialize(0)]] FIELD(Asset::SoundFile) bgm_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bgm_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
             archive(CEREAL_NVP(bgm_));
        }
#pragma endregion
        
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Sound::BgmPlayObject, 0);
CEREAL_REGISTER_TYPE(GamePlay::Sound::BgmPlayObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GamePlay::Sound::BgmPlayObject);
#pragma endregion