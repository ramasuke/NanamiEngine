#pragma once
#include <memory>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "../DrawGuiContext/PopupWindowDrawGuiContext.h"
#include "../Interface/IPopupWindow.h"

template <typename T>
concept PopupWindowType = std::derived_from<T, NanamiEngine::Core::PopupWindow::IPopupWindow>;

namespace NanamiEngine::Core::PopupWindow
{
    class PopupWindowGroup final
    {
    public:
        template <PopupWindowType WindowT>
        void MakeWindow();

        void InjectWindow(std::unique_ptr<IPopupWindow> window);

        template <PopupWindowType WindowT>
        [[nodiscard]] std::vector<WindowT*> Catch();
        void OnDraw(FileSystem::EditorDraggingHand& draggingHand);

    private:
        std::unordered_map<Guid, std::unique_ptr<IPopupWindow>, GuidHash> popupWindows_;
    };

    template <PopupWindowType WindowT>
    void PopupWindowGroup::MakeWindow()
    {
        auto window = std::make_unique<WindowT>();
        popupWindows_[window->Guid()] = std::move(window);
    }

    inline void PopupWindowGroup::InjectWindow(std::unique_ptr<IPopupWindow> window)
    {
        popupWindows_[window->Guid()] = std::move(window);
    }

    template <PopupWindowType WindowT>
    std::vector<WindowT*> PopupWindowGroup::Catch()
    {
        std::vector<WindowT*> result;

        for (const auto& window : popupWindows_ | std::views::values)
        {
            if (auto* casted = dynamic_cast<WindowT*>(window.get()); casted)
            {
                result.push_back(casted);
            }
        }

        if (result.empty())
        {
            MakeWindow<WindowT>();

            // 再検索して追加（1つだけ生成される前提）
            for (const auto& window : popupWindows_ | std::views::values)
            {
                if (auto* casted = dynamic_cast<WindowT*>(window.get()); casted)
                {
                    result.push_back(casted);
                    break;
                }
            }
        }

        return result;
    }

    inline void PopupWindowGroup::OnDraw(FileSystem::EditorDraggingHand& draggingHand)
    {
        std::vector<::Guid> closedWindows;

        for (auto& [guid, window] : popupWindows_)
        {
            if (!window)
                continue;

            if (window->OnDraw(PopupWindowDrawGuiContext(draggingHand)) == PopupWindowState::Closed)
                closedWindows.push_back(guid);
        }

        for (const auto& guid : closedWindows)
            popupWindows_.erase(guid);
    }
}
