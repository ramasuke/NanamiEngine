#include "CameraFollowTransform.h"

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../../../Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Component
{
    namespace
    {
        bool TryGetCameraPos(glm::vec3& outPos)
        {
            //NOTE: 編集中は CinemachineCameraBrain が起動していないので、エディタカメラを見る
            if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
            {
                outPos = Core::Application::ApplicationBase::GameWindow()->GetCameraPosition();
                return true;
            }

            const auto* cameraBrain = CineMachine::CinemachineCameraBrain::Instance();
            if (!cameraBrain)
                return false;

            outPos = cameraBrain->Transform().GetWorldPos();
            return true;
        }
    }

    void CameraFollowTransform::OnLateUpdate()
    {
        if (!IsEnable())
            return;

        glm::vec3 cameraPos;
        if (!TryGetCameraPos(cameraPos))
            return;

        const glm::vec3 target  = cameraPos + offset_;
        const glm::vec3 current = Transform().GetWorldPos();
        Transform().SetWorldPos(glm::vec3(
            followX_ ? target.x : current.x,
            followY_ ? target.y : current.y,
            followZ_ ? target.z : current.z));
    }

    void CameraFollowTransform::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("offset_",  offset_ );
        ImGuiHelper::OnDrawInputField("followX_", followX_);
        ImGuiHelper::OnDrawInputField("followY_", followY_);
        ImGuiHelper::OnDrawInputField("followZ_", followZ_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::CameraFollowTransform);
#pragma endregion
