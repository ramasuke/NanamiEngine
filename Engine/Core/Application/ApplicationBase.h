#pragma once
#include <memory>
#include <optional>
#include "Window/Main/Group/MainWindowGroup.h"

namespace NanamiEngine::Core::Application
{
    class ApplicationLifeCycle;
}

namespace NanamiEngine::Core::PopupWindow
{
    class PopupWindowGroup;
}

namespace NanamiEngine::Core
{
    class Physics;
}

namespace NanamiEngine::Core::MainWindow
{
    class GameWindow;
}

namespace NanamiEngine::Core::FileSystem
{
    class ObjectRegistry;
}

namespace NanamiEngine::Core::FileSystem
{
    class Directory;
}

namespace NanamiEngine::Core::Application
{
    class ApplicationBase
    {
    public:
        ApplicationBase();
        virtual ~ApplicationBase() = default;
        virtual void Run();
        virtual void OnExit() = 0;
        template <MainWindowType T>
        static void OnChangeWindow();
        static void OnChangeWindow(const std::shared_ptr<MainWindow::IMainWindow>& window);

        static MainWindow ::MainWindowGroup                  & MainWindows         () { return MainWindows_         (); }
        static PopupWindow::PopupWindowGroup                 & PopupWindows        () { return PopupWindows_        (); }
        static const std::shared_ptr<MainWindow::IMainWindow>& GetMainWindow       () { return CurrentMainWindow    (); }
        static FileSystem::Directory                         & AssetsDirectory     () { return AssetsDirectory_     (); }
        static ApplicationLifeCycle                          & ApplicationLifeCycle() { return ApplicationLifeCycle_(); }
        static FileSystem::ObjectRegistry                    & ObjectRegistry      () { return ObjectRegistry_      (); }
        static Physics                                       & Physics             ();
        static void                                            ResetPhysics        ();
        static std::shared_ptr<MainWindow::GameWindow>         GameWindow          (); 
        
    protected:
        static std::shared_ptr<MainWindow::IMainWindow>& CurrentMainWindow    ();
        static MainWindow::MainWindowGroup             & MainWindows_         ();
        static PopupWindow::PopupWindowGroup           & PopupWindows_        ();
        static FileSystem::Directory                   & AssetsDirectory_     ();
        static Application::ApplicationLifeCycle       & ApplicationLifeCycle_();
        static FileSystem::ObjectRegistry              & ObjectRegistry_      ();

        //TODO: optionalである必要ないです。
        static std::optional<Core::Physics> physics;
    };

    template <MainWindowType T>
    void ApplicationBase::OnChangeWindow()
    {
        CurrentMainWindow() = MainWindows_().Catch<T>();
    }
}
