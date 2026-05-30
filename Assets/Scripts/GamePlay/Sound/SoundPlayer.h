#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../Engine/Module/Component/AudioSource/AudioSource.h"

namespace GamePlay::Sound
{
    class SoundPlayer final : public Component::ComponentBase,
                              public LifeCycleCallback::IAwakable,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] static glm::vec3 Position();
        static void PlaySe(const Asset::SoundFile& sound, const glm::vec3& soundPosition);
        static void PlayBgm(const std::weak_ptr<Asset::SoundFile>& sound);
        static void StopAllBgm();
        static void StopBgm(const std::weak_ptr<Asset::SoundFile>& sound);

    private:
        void OnAwake() override;
        void OnUpdate() override;
        void OnDestroy() override;
        

        [[serialize(0)]] FIELD(Component::AudioSource) audioSource_;
        std::vector<std::weak_ptr<Asset::SoundFile>> bgmSounds_;
        static SoundPlayer* instance_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            instance_ = this;
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GamePlay::Sound::SoundPlayer, 0);
CEREAL_REGISTER_TYPE(GamePlay::Sound::SoundPlayer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, GamePlay::Sound::SoundPlayer);
#pragma endregion