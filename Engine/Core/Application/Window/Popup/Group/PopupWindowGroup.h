#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <memory>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "../DrawGuiContext/PopupWindowDrawGuiContext.h"
#include "../Interface/IPopupWindow.h"

template <typename T>
concept PopupWindowType = std::derived_from<T, NanamiEngine::Core::PopupWindow::IPopupWindow>;

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::PopupWindow
{
    class NANAMI_API PopupWindowGroup final
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        PopupWindowGroup() = default;
        // NOTE: export されたクラスは暗黙のコピーも実体化される。unique_ptr の map を持つので明示的に消す
        PopupWindowGroup(const PopupWindowGroup&)            = delete;
        PopupWindowGroup& operator=(const PopupWindowGroup&) = delete;

        template <PopupWindowType WindowT>
        void MakeWindow();

        void InjectWindow(std::unique_ptr<IPopupWindow> window);

        template <PopupWindowType WindowT>
        [[nodiscard]] std::vector<WindowT*> Catch();
        void OnDraw(FileSystem::EditorDraggingHand& draggingHand);

        /** @brief クラスが module にあるウィンドウを閉じて捨てる (ゲーム DLL を外す前)。戻り値は捨てた数 */
        std::size_t RemoveWindowsOfModule(const ModuleHandle module)
        {
            return std::erase_if(popupWindows_, [module](const auto& pair)
            {
                return pair.second && ModuleOfVTable(pair.second.get()) == module;
            });
        }

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
