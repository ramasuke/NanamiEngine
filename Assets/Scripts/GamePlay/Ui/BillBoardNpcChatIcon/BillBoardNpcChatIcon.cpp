#include "BillBoardNpcChatIcon.h"

#include "../../../Core/Game/Npc/Friendly/Behaviour/Action/Content/GameObject/Instantiate/Friendly_Behaviour_Action_GameObjectInstantaite.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include <cmath>

#include "../../../../../Engine/Core/Application/Time/Time.h"

namespace GamePlay::Ui
{
    namespace
    {
        void ApplyFloating(
            const std::shared_ptr<GameObject::IGameObject>& object,
            glm::vec3& basePos,
            bool& isBasePosCaptured,
            const float offset)
        {
            if (!object)
                return;

            // 浮遊オフセットを書き込む前でないと、揺れた後の座標を基準にしてしまう
            if (!isBasePosCaptured)
            {
                basePos = object->Transform().GetLocalPos();
                isBasePosCaptured = true;
            }

            auto position = basePos;
            position.y += offset;
            object->Transform().SetLocalPos(position);
        }
    }

    void BillBoardNpcChatIcon::Show(
        const bool chattableIcon,
        const bool chattingIcon,
        const bool surpriseIcon)
    {
        isShow_ = true;

        if (chattableIcon_) chattableIcon_->SetEnable(chattableIcon);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(chattingIcon);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(surpriseIcon);
    }

    void BillBoardNpcChatIcon::Hide()
    {
        isShow_ = false;

        if (chattableIcon_) chattableIcon_->SetEnable(false);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(false);
        if (surpriseIcon_)  surpriseIcon_ ->SetEnable(false);
    }

    void BillBoardNpcChatIcon::OnChattable()
    {
        if (!isShow_)
            return;

        if (chattableIcon_) chattableIcon_->SetEnable(false);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(true);
    }

    void BillBoardNpcChatIcon::OnExitChattable()
    {
        if (!isShow_)
            return;

        if (chattableIcon_) chattableIcon_->SetEnable(true);
        if (chattingIcon_)  chattingIcon_ ->SetEnable(false);
    }

    void BillBoardNpcChatIcon::OnUpdate()
    {
        // ビックリマークは SetEnableShowChatIcon ではなく GameObjectSetEnable で
        // 直接表示されるため、isShow_ で揺れを止めてはいけない
        const float time = Time::CurrentTime();

        constexpr float amplitude = 0.2f;
        constexpr float speed     = 2.0f;

        const float offset = std::sin(time * speed) * amplitude;

        ApplyFloating(chattableIcon_.get(), basePosChattable_, isBasePosChattableCaptured_, offset);
        ApplyFloating(surpriseIcon_ .get(), basePosSurprise_ , isBasePosSurpriseCaptured_ , offset);
    }

    void BillBoardNpcChatIcon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chattableIcon_", chattableIcon_);
        ImGuiHelper::OnDrawInputField("chattingIcon_", chattingIcon_);
        ImGuiHelper::OnDrawInputField("surpriseIcon_", surpriseIcon_);
    }
}