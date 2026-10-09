#include "SoundPlayer.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Sound
{
    SoundPlayer* SoundPlayer::instance_ = nullptr;

    SoundPlayer::~SoundPlayer()
    {
        if (instance_ == this)
        {
            instance_ = nullptr;
        }
    }

    glm::vec3 SoundPlayer::Position()
    {
        if (!instance_)
            return glm::vec3(0.0f);

        return instance_->Transform().GetWorldPos();
    }

    void SoundPlayer::PlaySe(const Asset::SoundFile& sound, const glm::vec3& soundPosition)
    {
        if (!instance_ || !instance_->audioSource_)
            return;

        instance_->audioSource_->Play(sound, soundPosition);
    }

    void SoundPlayer::PlayBgm(const std::weak_ptr<Asset::SoundFile>& sound, const float fadeIn_secs)
    {
        if (!instance_)
            return;

        const auto soundFile = sound.lock();
        if (!soundFile)
        {
            // NOTE: BGM が解決できていないと無音になるだけなので、警告を残す
            Module::LogWarning("SoundPlayer: BGM が設定されていないか、読み込めていません");
            return;
        }

        instance_->CancelFade(soundFile, true);
        if (fadeIn_secs > 0.0f)
        {
            soundFile->SetVolume(0);
            instance_->bgmFades_.push_back({ soundFile, 0, soundFile->GetVolume(), 0.0f, fadeIn_secs, false });
        }

        instance_->audioSource_ = instance_->RequireComponent<Component::AudioSource>();

        instance_->bgmSounds_.push_back(soundFile);
        instance_->audioSource_->SetLoop(true);
        instance_->audioSource_->Play(*soundFile, instance_->Transform().GetWorldPos());
        instance_->audioSource_->SetLoop(false);
    }

    void SoundPlayer::StopAllBgm()
    {
        if (!instance_)
            return;

        const auto bgmSounds = instance_->bgmSounds_;
        for (const auto& bgm : bgmSounds)
        {
            StopBgm(bgm);
        }
        instance_->bgmSounds_.clear();

        for (const auto& fade : instance_->bgmFades_)
        {
            if (const auto fading = fade.sound.lock())
            {
                fading->Stop();
                fading->SetVolume(fading->GetVolume());
            }
        }
        instance_->bgmFades_.clear();
    }

    void SoundPlayer::FadeOutAllBgm(const float fadeOut_secs)
    {
        if (!instance_)
            return;

        if (fadeOut_secs <= 0.0f)
        {
            StopAllBgm();
            return;
        }

        for (const auto& bgm : instance_->bgmSounds_)
        {
            const auto soundFile = bgm.lock();
            if (!soundFile)
                continue;

            instance_->CancelFade(soundFile, false);
            instance_->bgmFades_.push_back({ soundFile, soundFile->GetCurrentVolume(), 0, 0.0f, fadeOut_secs, true });
        }
        // NOTE: 下げている間に次の BGM を PlayBgm できるよう、鳴っている BGM の扱いから外す
        instance_->bgmSounds_.clear();
    }

    void SoundPlayer::StopBgm(const std::weak_ptr<Asset::SoundFile>& sound)
    {
        if (!instance_)
            return;

        const auto soundTarget = sound.lock();
        if (!soundTarget)
            return;

        soundTarget->Stop();
        instance_->CancelFade(soundTarget, true);

        auto& list = instance_->bgmSounds_;
        list.erase(
            std::ranges::remove_if(list,
               [&](const std::weak_ptr<Asset::SoundFile>& s)
               {
                   const auto locked = s.lock();
                   return !locked || locked == soundTarget;
               }
            ).begin(),
            list.end()
        );
    }

    void SoundPlayer::OnAwake()
    {
        audioSource_ = RequireComponent<Component::AudioSource>();
    }

    void SoundPlayer::OnUpdate()
    {
        for (const auto& bgmSound : bgmSounds_)
        {
            if (const auto bgmSoundFile = bgmSound.lock())
                bgmSoundFile->Set3DPosition(Transform().GetWorldPos());
        }

        UpdateFades();
    }

    void SoundPlayer::UpdateFades()
    {
        for (auto it = bgmFades_.begin(); it != bgmFades_.end();)
        {
            const auto soundFile = it->sound.lock();
            if (!soundFile)
            {
                it = bgmFades_.erase(it);
                continue;
            }

            it->elapsed_secs += Time::DeltaTime();
            const float rate = std::clamp(it->elapsed_secs / it->duration_secs, 0.0f, 1.0f);
            soundFile->SetVolume(it->fromVolume + static_cast<int>(static_cast<float>(it->toVolume - it->fromVolume) * rate));

            if (rate < 1.0f)
            {
                ++it;
                continue;
            }

            if (it->stopAtEnd)
            {
                soundFile->Stop();
                soundFile->SetVolume(soundFile->GetVolume());
            }
            it = bgmFades_.erase(it);
        }
    }

    void SoundPlayer::CancelFade(const std::shared_ptr<Asset::SoundFile>& sound, const bool restoreVolume)
    {
        const auto removed = std::erase_if(bgmFades_, [&](const BgmFade& fade)
        {
            const auto locked = fade.sound.lock();
            return !locked || locked == sound;
        });

        if (removed > 0 && restoreVolume)
            sound->SetVolume(sound->GetVolume());
    }

    void SoundPlayer::OnDestroy()
    {
        if (instance_ == this)
        {
            StopAllBgm();
            instance_ = nullptr;
        }
    }

    void SoundPlayer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("audioSource_", audioSource_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Sound::SoundPlayer);
#pragma endregion
