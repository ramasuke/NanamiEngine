#include "UiFlow_ScreenStack.h"

#include <algorithm>

#include "UiFlow_UiScreen.h"
#include "../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"

namespace NanamiEngine::UiFlow
{
    namespace
    {
        // UiScreen の下にある Button は、その画面が最前面のときだけ押せる。UiScreen の下に無い Button は常に押せる
        bool IsButtonOnFocusedScreen(const Module::NanamiUi::Button& button)
        {
            auto gameObject = button.Entity().lock();
            while (gameObject)
            {
                if (const auto screen = gameObject->Components().Catch<UiScreen>().lock())
                    return screen->IsFocused();
                gameObject = gameObject->Transform().GetParent();
            }
            return true;
        }

        struct ButtonInputGateRegistrar
        {
            ButtonInputGateRegistrar() { Module::NanamiUi::Button::SetInputGate(&IsButtonOnFocusedScreen); }
        };
        const ButtonInputGateRegistrar buttonInputGateRegistrar;
    }

    ScreenStack& ScreenStack::Instance()
    {
        static ScreenStack instance;
        return instance;
    }

    UiScreen* ScreenStack::Top() const
    {
        return screens_.empty() ? nullptr : screens_.back();
    }

    bool ScreenStack::IsOpen(const std::string_view screenId) const
    {
        return std::ranges::any_of(screens_, [screenId](const UiScreen* screen)
        {
            return screen->ScreenId() == screenId;
        });
    }

    std::vector<std::string> ScreenStack::ScreenIds() const
    {
        std::vector<std::string> ids;
        ids.reserve(screens_.size());
        for (const auto* screen : screens_)
        {
            ids.push_back(screen->ScreenId());
        }
        return ids;
    }

    void ScreenStack::Clear()
    {
        screens_.clear();
    }

    void ScreenStack::Push(UiScreen& screen)
    {
        screens_.push_back(&screen);
    }

    void ScreenStack::Remove(UiScreen& screen)
    {
        std::erase(screens_, &screen);
    }
}
