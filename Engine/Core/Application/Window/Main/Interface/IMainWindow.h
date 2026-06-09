#pragma once
#include "../../LifeCycle/WindowLifeCycle.h"
#include "../DrawGuiContext/MainWindowDrawGuiContext.h"

namespace NanamiEngine::Core::MainWindow
{
    class IMainWindow
    {
    public:
        virtual ~IMainWindow() = default;
        virtual void OnUpdate() = 0;
        [[nodiscard]] virtual Application::WindowLifeCycle& LifeCycle() = 0;
        virtual void OnDrawGui(MainWindowDrawGuiContext context) = 0;
        virtual void OnSave() = 0;
    };
    
    template <typename T>
    concept MainWindowType = std::derived_from<T, NanamiEngine::Core::MainWindow::IMainWindow>;
}
