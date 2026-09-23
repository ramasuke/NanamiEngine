#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/LookAt/VirtualCameraLookAtBehaviour.h"

namespace
{
    // 汎用演出RPC: シーン上の VirtualCamera(Guid 指定)の優先度を変更する
    struct ScenePurposeCameraRpcRegistration
    {
        ScenePurposeCameraRpcRegistration()
        {
            GameCore::Network::ScenePurposeCameraRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& sender, Guid cameraGuid, int priority)
                {
                    const auto camera = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::CineMachine::CineMachineVirtualCamera>(cameraGuid).lock();
                    if (!camera)
                    {
                        NanamiEngine::Module::LogWarning("ScenePurposeCameraRpc: CineMachineVirtualCamera が見つかりません (guid:" + cameraGuid.Value() + ")");
                        return;
                    }
                    camera->SetPriority(priority);

                    // NOTE: 権威側と同じく、注視先が空のLookAtは送り元の敵を追いかける
                    if (const auto lookAt = camera->Components().Catch<NanamiEngine::CineMachine::Behaviour::VirtualCameraLookAtBehaviour>().lock();
                        lookAt && !lookAt->HasTarget())
                    {
                        lookAt->SetTarget(sender.Entity().lock());
                    }
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static ScenePurposeCameraRpcRegistration s_scenePurposeCameraRpcRegistration;
}
