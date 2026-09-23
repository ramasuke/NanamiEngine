#include "BgmPlayer.h"

#include "../SoundPlayer.h"
#include "Engine/Module/Component/AudioSource/AudioSource.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Sound
{
    void BgmPlayObject::OnAwake()
    {
        RequireComponent<SoundPlayer>();
    }

    void BgmPlayObject::OnStart()
    {
        SoundPlayer::PlayBgm(bgm_.get());
    }

    void BgmPlayObject::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Sound::BgmPlayObject);
#pragma endregion
