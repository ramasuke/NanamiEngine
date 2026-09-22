#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace
{
    // 汎用演出RPC: 宛先 NetworkObject の子オブジェクト(名前指定)が持つ VirtualCamera の優先度を変更する
    struct PurposeCameraRpcRegistration
    {
        PurposeCameraRpcRegistration()
        {
            GameCore::Network::PurposeCameraRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& networkGameObject, std::string childCameraName, int priority)
                {
                    const auto child = networkGameObject.Transform().CatchChild(childCameraName);
                    if (!child)
                    {
                        NanamiEngine::Module::LogWarning("PurposeCameraRpc: 子オブジェクトが見つかりません (name:" + childCameraName + ")");
                        return;
                    }
                    const auto camera = child->Components().Catch<NanamiEngine::CineMachine::CineMachineVirtualCamera>().lock();
                    if (!camera)
                    {
                        NanamiEngine::Module::LogWarning("PurposeCameraRpc: CineMachineVirtualCamera がありません (name:" + childCameraName + ")");
                        return;
                    }
                    camera->SetPriority(priority);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PurposeCameraRpcRegistration s_purposeCameraRpcRegistration;
}
