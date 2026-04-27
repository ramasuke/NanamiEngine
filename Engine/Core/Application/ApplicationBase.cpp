#include "ApplicationBase.h"

#include "EffekseerForDXLib.h"
#include "../FileSystem/Directory/Directory.h"
#include "../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"
#include "Window/Main/MainWindowBase.h"
#include "Window/Main/Game/GameWindow.h"
#include "../Object/Registry/ObjectRegistry.h"
#include "Configuration/ApplicationConfiguration.h"
#include "Time/Time.h"
#include "../Physics/Physics.h"
#include "LifeCycle/ApplicationLifeCycle.h"
#include "TweenManager/TweenManager.h"
#include "Window/Popup/Group/PopupWindowGroup.h"

namespace NanamiEngine::Core::Application
{
    ApplicationBase::ApplicationBase()
    {
        /** ApplicationConfiguの初期化 */
        ChangeWindowMode     (false          );
        SetGraphMode         (Configuration::WINDOW_WIDTH_SIZE, Configuration::WINDOW_HEIGHT_SIZE, Configuration::WINDOW_COLOR_SCALE);
        SetUseDirect3DVersion(DX_DIRECT3D_11);
        SetUseZBuffer3D      (TRUE          );
        SetWriteZBuffer3D    (TRUE          );
        SetDrawScreen        (DX_SCREEN_BACK);
        SetUseIMEFlag        (TRUE          );
        DxLib_Init           (              );

        /** リソースの初期化 */
        SetUseASyncLoadFlag(true);
        AssetsDirectory_   (    );

        /** Windowの初期化 */
        SetUseSetDrawScreenSettingReset(false);
        MainWindows_().MakeWindow<MainWindow::GameWindow>();
        OnChangeWindow<MainWindow::GameWindow>();

        /** Sceneの初期化 */
        const auto initScene = std::make_shared<Scene::Scene>("Assets/Scene/GameManage.scene");
        MainWindows().Catch<MainWindow::GameWindow>()->AddContent     (initScene);
        MainWindows().Catch<MainWindow::GameWindow>()->ChangeMainScene(initScene);

        Physics().Initialize();

        /** Effekseerの初期化 */
        Effekseer_Init(8000);
        SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
        Effekseer_SetGraphicsDeviceLostCallbackFunctions();
    }
    
    void ApplicationBase::Run()
    {
        Time::Update();
    }
    
    void ApplicationBase::OnChangeWindow(const std::shared_ptr<MainWindow::IMainWindow>& window)
    {
        CurrentMainWindow() = window;
    }
    
    std::shared_ptr<MainWindow::IMainWindow>& ApplicationBase::
    CurrentMainWindow()
    {
        static std::shared_ptr<MainWindow::IMainWindow> currentMainWindow = nullptr;
        return currentMainWindow;
    }
    
    MainWindow::MainWindowGroup& ApplicationBase::MainWindows_()
    {
        static MainWindow::MainWindowGroup mainWindows;
        return mainWindows;
    }
    
    PopupWindow::PopupWindowGroup& ApplicationBase::PopupWindows_()
    {
        static PopupWindow::PopupWindowGroup popupWindows;
        return popupWindows;
    }
    
    FileSystem::Directory& ApplicationBase::AssetsDirectory_()
    {
        static auto assetsDirectory = FileSystem::Directory("Assets");
        return assetsDirectory;
    }
    
    ApplicationLifeCycle& ApplicationBase::ApplicationLifeCycle_()
    {
        static auto applicationLifeCycle = Application::ApplicationLifeCycle();
        return applicationLifeCycle;
    }
    
    FileSystem::ObjectRegistry& ApplicationBase::ObjectRegistry_()
    {
        static FileSystem::ObjectRegistry assetRegistry;
        return assetRegistry;
    }
    
    Physics& ApplicationBase::Physics_()
    {
        static Core::Physics physics;
        return physics;
    }
    
    TweenManager& ApplicationBase::TweenManager_()
    {
        static Core::TweenManager tweenManager;
        return tweenManager;
    }
    
    std::shared_ptr<MainWindow::GameWindow> ApplicationBase::GameWindow()
    {
        static std::shared_ptr<MainWindow::GameWindow> gameWindow = MainWindows().Catch<MainWindow::GameWindow>();;
        return gameWindow;
    }
} 
