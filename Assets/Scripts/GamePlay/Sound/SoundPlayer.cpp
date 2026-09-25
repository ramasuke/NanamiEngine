#include "SoundPlayer.h"

#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Sound
{
    SoundPlayer* SoundPlayer::instance_ = nullptr;

    SoundPlayer::~SoundPlayer()
    {
        // NOTE: RemoveComponent は OnDestroy を呼ばないので、デストラクタでも解除する
        if (instance_ == this)
            instance_ = nullptr;
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

    void SoundPlayer::PlayBgm(const std::weak_ptr<Asset::SoundFile>& sound)
    {
        if (!instance_)
            return;

        const auto soundFile = sound.lock();
        if (!soundFile)
            return;

        instance_->audioSource_ = instance_->RequireComponent<Component::AudioSource>();

        instance_->bgmSounds_.push_back(sound);
        instance_->audioSource_->SetLoop(true);
        instance_->audioSource_->Play(*soundFile, instance_->Transform().GetWorldPos());
        instance_->audioSource_->SetLoop(false);
    }

    void SoundPlayer::StopAllBgm()
    {
        if (!instance_)
            return;

        // NOTE: StopBgm が bgmSounds_ から erase するので、コピーを回す
        const auto bgmSounds = instance_->bgmSounds_;
        for (const auto& bgm : bgmSounds)
        {
            StopBgm(bgm);
        }
        // NOTE: 期限切れの weak_ptr は StopBgm が早期 return して残るので捨てる
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
        for (const auto& bgmSound : bgmSounds_)
        {
            if (const auto bgmSoundFile = bgmSound.lock())
                bgmSoundFile->Set3DPosition(Transform().GetWorldPos());
        }
    }

    void SoundPlayer::OnDestroy()
    {
        if (instance_ == this)
            instance_ = nullptr;
    }

    void SoundPlayer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("audioSource_", audioSource_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Sound::SoundPlayer);
#pragma endregion
