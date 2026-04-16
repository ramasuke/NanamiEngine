#include "FriendlyNpc.h"

#include "../../../Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.h"

namespace GamePlay::Npc::Friendly
{
    FriendlyNpc::FriendlyNpc () = default;
    FriendlyNpc::~FriendlyNpc() = default;

    void FriendlyNpc::OnAwake()
    {
         behaviour_ = friendlyNpcBehaviourFile_->OnLoadCopyContent();
    }

    void FriendlyNpc::OnUpdate()
    {
        behaviour_->Tick(name_,
                         Entity(),
                        isChatting_);
    }

    void FriendlyNpc::OnChat()
    {
        isChatting_ = true;
    }

    const GameObject::Transform& FriendlyNpc::ChattableTransform() const
    {
        return Transform();
    }

    void FriendlyNpc::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("name_"                    , name_);
        ImGuiHelper::OnDrawInputField("friendlyNpcBehaviourFile_", friendlyNpcBehaviourFile_);
    }
}
