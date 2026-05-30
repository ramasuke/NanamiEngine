#include "NanamiUi_Button.h"
#include <DxLib.h>

#include "../NanamiUi_IInteractivableRenderer.h"
#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module
{
    void NanamiUi::Button::OnAwake()
    {
        renderer_ = Components().Catch<IInteractivableRenderer>();  
    }

    void NanamiUi::Button::OnUpdate()
    {
        TryHover();
        TryClick();
        TryRelease();
    }

    void NanamiUi::Button::TryClick()
    {
        if (!CheckInnerMousePointer() || isPressing_)
            return;

        MouseState state{};
        bool isClick = false;

        switch (GetMouseInput())
        {
        case MOUSE_INPUT_LEFT:
            state = MouseState::LeftClick;
            isClick = true;
            break;
        case MOUSE_INPUT_MIDDLE:
            state = MouseState::MiddleClick;
            isClick = true;
            break;
        case MOUSE_INPUT_RIGHT:
            state = MouseState::RightClick;
            isClick = true;
            break;
        default:
            break;
        }

        if (!isClick)
            return;

        isPressing_ = true;
        onClick.get_subscriber().on_next(state);
    }
    
    void NanamiUi::Button::TryHover()
    {
        bool isInside = CheckInnerMousePointer();

        // 入った瞬間
        if (isInside && !isHovering_)
        {
            isHovering_ = true;
            onHover.get_subscriber().on_next(Rx::unit{});
            if (const auto renderer = renderer_.lock())
                renderer->SetSprite(onHoverSprite_.get());
        }
        // 出た瞬間
        else if (!isInside && isHovering_)
        {
            isHovering_ = false;
            if (const auto renderer = renderer_.lock())
                renderer->SetSprite(onIdleSprite_.get());
        }
    }

    void NanamiUi::Button::TryRelease()
    {
        if (!isPressing_)
            return;

        if (GetMouseInput() != 0)
            return;

        isPressing_ = false;
        onRelease.get_subscriber().on_next(Rx::unit{});
    }

    void NanamiUi::Button::OnDrawGui()
    {
        float evevntArea[2] = { eventAreaSize_.x, eventAreaSize_.y };
        if (ImGui::InputFloat2("eventAreaSize", evevntArea))
        {
            eventAreaSize_.x = evevntArea[0];
            eventAreaSize_.y = evevntArea[1];
        }
        const auto& position = Transform().GetWorldPos();
        DrawBox(
            position.x - eventAreaSize_.x,
            position.y - eventAreaSize_.y,
            position.x + eventAreaSize_.x,
            position.y + eventAreaSize_.y,
            GetColor(0, 255, 0),
            false
        );
        ImGuiHelper::OnDrawInputField("onIdleSprite_", onIdleSprite_);
        ImGuiHelper::OnDrawInputField("onHoverSprite_", onHoverSprite_);
    }

    bool NanamiUi::Button::CheckInnerMousePointer() const
    {
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);

        const auto& position = Transform().GetWorldPos();

        return position.x - eventAreaSize_.x < mouseX &&
               position.x + eventAreaSize_.x > mouseX &&
               position.y - eventAreaSize_.y < mouseY &&
               position.y + eventAreaSize_.y > mouseY;
    }
}
