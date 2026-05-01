#include "BillBoardNpcChatIcon.h"

#include "../../../Core/Game/Npc/Friendly/Behaviour/Action/Content/GameObject/Instantiate/Friendly_Behaviour_Action_GameObjectInstantaite.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include <cmath>

#include "../../../../../Engine/Core/Application/Time/Time.h"

namespace GamePlay::Ui
{
    void BillBoardNpcChatIcon::Show()
    {
        isShow_ = true;
        
        chattableIcon_->SetEnable(true);
        chattingIcon_ ->SetEnable(true);   
    }

    void BillBoardNpcChatIcon::Hide()
    {
        isShow_ = false;
        
        chattableIcon_->SetEnable(false);
        chattingIcon_ ->SetEnable(false);
    }

    void BillBoardNpcChatIcon::OnChattable()
    {
        if (!isShow_)
            return;
        
        chattableIcon_->SetEnable(false);
        chattingIcon_ ->SetEnable(true);
    }

    
    void BillBoardNpcChatIcon::OnExitChattable()
    {
        if (!isShow_)
            return;
        
        chattableIcon_->SetEnable(true);
        chattingIcon_ ->SetEnable(false);
    }

    void BillBoardNpcChatIcon::OnAwake()
    {
        basePos_ = chattableIcon_->TransformRef().GetLocalPos();
    }

    void BillBoardNpcChatIcon::OnUpdate()
    {
        if (!chattableIcon_)
            return;

        const float time = Time::CurrentTime();

        constexpr float amplitude = 0.2f;
        constexpr float speed     = 2.0f;

        auto position = basePos_;
        position.y += std::sin(time * speed) * amplitude;

        chattableIcon_->TransformRef().SetLocalPos(position);
    }

    void BillBoardNpcChatIcon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chattableIcon_", chattableIcon_);
        ImGuiHelper::OnDrawInputField("chattingIcon_", chattingIcon_);
    }
}
