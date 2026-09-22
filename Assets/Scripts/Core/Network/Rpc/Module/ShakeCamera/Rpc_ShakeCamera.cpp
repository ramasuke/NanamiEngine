#include "../../Custom_RpcType.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"

namespace
{
    // 汎用演出RPC: 権威側が起こしたカメラシェイクを、この宛先を持つ他ピア自身のカメラにも反映する
    struct ShakeCameraRpcRegistration
    {
        ShakeCameraRpcRegistration()
        {
            GameCore::Network::ShakeCameraRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, float intensity, float duration)
                {
                    NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(intensity, duration);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static ShakeCameraRpcRegistration s_shakeCameraRpcRegistration;
}
