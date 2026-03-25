#include "SoundPlayer.h"

#include "DxLib.h"
#include "../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Sound
{
    SoundPlayer* SoundPlayer::instance_ = nullptr;
    
    void SoundPlayer::PlaySe(const Asset::SoundFile& sound, const glm::vec3& soundPosition)
    {
        instance_->audioSource_->Play(sound, soundPosition);
    }

    void SoundPlayer::PlayBgm(const std::weak_ptr<Asset::SoundFile>& sound)
    {
        instance_->audioSource_ = instance_->RequireComponent<Component::AudioSource>();
        
        instance_->bgmSounds_.push_back(sound);
        instance_->audioSource_->SetLoop(true);
        instance_->audioSource_->Play(*sound.lock(), instance_->Transform().GetWorldPos());
        instance_->audioSource_->SetLoop(false);
    }

    void SoundPlayer::StopAllBgm()
    {
        for (const auto& bgm : instance_->bgmSounds_)
        {
            StopBgm(bgm);
        }
    }

    void SoundPlayer::StopBgm(const std::weak_ptr<Asset::SoundFile>& sound)
    {
        if (!instance_)
            return;

        const auto soundTarget = sound.lock();
        if (!soundTarget)
            return;

        StopSoundMem(soundTarget->GetDxLibHandle());

        // 管理リストから削除
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
        for (auto& bgmSoundFile: instance_->bgmSounds_)
        {
            Set3DPositionSoundMem(Transform().GetDxWorldPos(), bgmSoundFile.lock()->GetDxLibHandle());
        }
    }

    void SoundPlayer::OnDestroy()
    {
        
    }

    void SoundPlayer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("audioSource_", audioSource_);
    }
}
