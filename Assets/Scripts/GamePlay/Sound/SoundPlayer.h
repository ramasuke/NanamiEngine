#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/AudioSource/AudioSource.h"

namespace GamePlay::Sound
{
    class SoundPlayer final : public Component::ComponentBase,
                              public LifeCycleCallback::IAwakable,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        ~SoundPlayer() override;

        [[nodiscard]] static glm::vec3 Position();
        static void PlaySe(const Asset::SoundFile& sound, const glm::vec3& soundPosition);
        /** @brief fadeIn_secs が 0 より大きいと、無音からアセットの音量まで上げる */
        static void PlayBgm(const std::weak_ptr<Asset::SoundFile>& sound, float fadeIn_secs = 0.0f);
        static void StopAllBgm();
        static void StopBgm(const std::weak_ptr<Asset::SoundFile>& sound);
        /** @brief 鳴っている BGM を全て fadeOut_secs かけて下げてから止める。0 以下ならすぐ止める */
        static void FadeOutAllBgm(float fadeOut_secs);

    private:
        struct BgmFade
        {
            std::weak_ptr<Asset::SoundFile> sound;
            int   fromVolume    = 0;
            int   toVolume      = 0;
            float elapsed_secs  = 0.0f;
            float duration_secs = 0.0f;
            bool  stopAtEnd     = false;
        };

        void OnAwake() override;
        void OnUpdate() override;
        void OnDestroy() override;
        void UpdateFades();
        void CancelFade(const std::shared_ptr<Asset::SoundFile>& sound, bool restoreVolume);

        [[serialize(0)]] FIELD(Component::AudioSource) audioSource_;
        std::vector<std::weak_ptr<Asset::SoundFile>> bgmSounds_;
        std::vector<BgmFade> bgmFades_;
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

CEREAL_CLASS_VERSION(GamePlay::Sound::SoundPlayer, 0);
