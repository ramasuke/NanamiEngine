#include "AudioSource.h"

#include "DxLib.h"
#include "../../Asset/Sound/SoundFile.h"
#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::Component
{
    void AudioSource::Play(const Asset::SoundFile& soundFile, const glm::vec3& soundPos) const
    {
        Set3DRadiusSoundMem(listenableRadius_, soundFile.GetDxLibHandle());
        Set3DPositionSoundMem({soundPos.x, soundPos.y, soundPos.z}, soundFile.GetDxLibHandle());
        
        if (isLoop_)
        {
            PlaySoundMem(soundFile.GetDxLibHandle(), DX_PLAYTYPE_LOOP);
        }
        else
        {
            PlaySoundMem(soundFile.GetDxLibHandle(), DX_PLAYTYPE_BACK);
        }
    }

    void AudioSource::SetLoop(const bool loop)
    {
        isLoop_ = loop;
    }

    void AudioSource::OnAwake()
    {
        
    }

    void AudioSource::OnUpdate()
    {
        const auto soundListenAngle = Transform().GetWorldEulerAngle();
        Set3DSoundListenerPosAndFrontPos_UpVecY(Transform().GetDxWorldPos(), VECTOR(soundListenAngle.x, soundListenAngle.y, soundListenAngle.z));
    }

    void AudioSource::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isRoop_", isLoop_);
        ImGui::SliderInt("volume_", &volume_, 0, 255);
    }
}
