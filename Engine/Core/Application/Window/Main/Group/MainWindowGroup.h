#pragma once
#include <ranges>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "../Interface/IMainWindow.h"

template <typename T>
concept MainWindowType = std::derived_from<T, NanamiEngine::Core::MainWindow::IMainWindow>;

namespace NanamiEngine::Core::MainWindow
{
    class MainWindowGroup final
    {
    public:
        template <MainWindowType WindowT>
        void MakeWindow();
        void OnSave();

        template <MainWindowType WindowT>
        [[nodiscard]] std::shared_ptr<WindowT> Catch() const;

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
