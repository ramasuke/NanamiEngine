#include "FriendlyNpc.h"

#include "../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Npc/Friendly/Behaviour/Friendly_BehaviourTree.h"
#include "../../../Editor/Npc/Friendly/Behaviour/Window/RunningFriendlyBehaviourTreeWindow.h"

namespace GamePlay::Npc::Friendly
{
    FriendlyNpc::FriendlyNpc () = default;
    FriendlyNpc::~FriendlyNpc() = default;

    void FriendlyNpc::OnAwake()
    {
        if (friendlyNpcBehaviourFile_)
            behaviour_ = friendlyNpcBehaviourFile_->OnLoadCopyContent();
    }

    void FriendlyNpc::OnUpdate()
    {
        // BehaviourTree が未設定・読み込み失敗（OnLoadCopyContent が nullptr）の場合は何もしない
        if (!behaviour_)
            return;

        behaviour_->Tick(name_,
                         Entity(),
                         billboardNpcChatIcon_.get(),
                        isChatting_);
    }

    void FriendlyNpc::OnChattable()
    {
        billboardNpcChatIcon_->OnChattable();
    }

    void FriendlyNpc::OnExitChattable()
    {
        billboardNpcChatIcon_->OnExitChattable();
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
        ImGuiHelper::OnDrawInputField("name_"                    , name_                    );
        ImGuiHelper::OnDrawInputField("friendlyNpcBehaviourFile_", friendlyNpcBehaviourFile_);
        ImGuiHelper::OnDrawInputField("baseStatus_"              , baseStatus_              );
        ImGuiHelper::OnDrawInputField("billboardNpcChatIcon_"    , billboardNpcChatIcon_    );

        if (behaviour_ && ImGui::Button("実行中のBehaviourTreeを表示"))
        {
            for (auto* window : Core::Application::ApplicationBase::PopupWindows().Catch<Editor::Npc::Friendly::RunningFriendlyBehaviourTreeWindow>())
                window->TryAddTarget(behaviour_);
        }
    }
}
