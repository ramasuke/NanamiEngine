#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <ranges>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "../Interface/IMainWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    class NANAMI_API MainWindowGroup final
    {
    public:
        template <MainWindowType WindowT>
        void MakeWindow();
        void OnSave();

        template <MainWindowType WindowT>
        [[nodiscard]] std::shared_ptr<WindowT> Catch() const;

        /** @brief クラスが module にあるウィンドウの実体を捨てる (ゲーム DLL を外す前)。戻り値は捨てた数 */
        std::size_t RemoveWindowsOfModule(const void* module);
        /** @brief window のクラスが module にあるか */
        [[nodiscard]] static bool IsWindowOfModule(const IMainWindow* window, const void* module);

    private:
        std::unordered_map<std::type_index, std::shared_ptr<IMainWindow>> mainWindows_;
    };

    inline void MainWindowGroup::OnSave()
    {
        for (const auto& window : mainWindows_ | std::views::values)
        {
            window->OnSave();
        }
    }

    inline std::size_t MainWindowGroup::RemoveWindowsOfModule(const void* module)
    {
        return std::erase_if(mainWindows_, [module](const auto& pair) { return IsWindowOfModule(pair.second.get(), module); });
    }

    inline bool MainWindowGroup::IsWindowOfModule(const IMainWindow* window, const void* module)
    {
        return window != nullptr && ModuleOfVTable(window) == module;
    }

    template <MainWindowType WindowT>
    void MainWindowGroup::MakeWindow()
    {
        mainWindows_[typeid(WindowT)] = std::make_shared<WindowT>();
    }

    template <MainWindowType WindowT>
    std::shared_ptr<WindowT> MainWindowGroup::Catch() const
    {
        if (const auto it = mainWindows_.find(typeid(WindowT)); it != mainWindows_.end())
        {
            std::shared_ptr<IMainWindow> basePtr = it->second;
            WindowT* rawPtr = dynamic_cast<WindowT*>(basePtr.get());
            return std::shared_ptr<WindowT>(basePtr, rawPtr);
        }

        const_cast<MainWindowGroup*>(this)->MakeWindow<WindowT>();
        if (const auto it = mainWindows_.find(typeid(WindowT)); it != mainWindows_.end())
        {
            std::shared_ptr<IMainWindow> basePtr = it->second;
            WindowT* rawPtr = dynamic_cast<WindowT*>(basePtr.get());
            return std::shared_ptr<WindowT>(basePtr, rawPtr);
        }
        return nullptr;
    }
}
