#include "SpawnSound.h"

#include "SoundPlayer.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Sound
{
    void SpawnSound::OnUpdate()
    {
        if (hasPlayed_)
            return;

        elapsed_secs_ += Time::DeltaTime();
        if (elapsed_secs_ < delay_secs_)
            return;

        hasPlayed_ = true;
        if (sound_)
            SoundPlayer::PlaySe(*sound_.get(), Transform().GetWorldPos());
    }

    void SpawnSound::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sound_", sound_);
        ImGuiHelper::OnDrawInputField("delay_secs_", delay_secs_);
        ImGui::Text("played: %s", hasPlayed_ ? "true" : "false");
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Sound::SpawnSound);
#pragma endregion
