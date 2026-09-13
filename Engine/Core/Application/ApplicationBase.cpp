#include "ApplicationBase.h"

#include "EffekseerForDXLib.h"
#include "../../Module/Network/Engine_Network_NetworkRunner.h"
#include "../../Module/Scene/GameObject/Helper/GameObject.h"
#include "../FileSystem/Directory/Directory.h"
#include "../../Module/Scene/GameObject/SceneGameObject/SceneGameObject.h"
#include "../Network/Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "Window/Main/MainWindowBase.h"
#include "Window/Main/Game/GameWindow.h"
#include "../Object/Registry/ObjectRegistry.h"
#include "Configuration/ApplicationConfiguration.h"
#include "Configuration/Network/ApplicationConfiguration_Network.h"
#include "Configuration/Physics/ApplicationConfiguration_Physics.h"
#include "Time/Time.h"
#include "../Physics/Physics.h"
#include "LifeCycle/ApplicationLifeCycle.h"
#include "Window/Popup/Group/PopupWindowGroup.h"

namespace
{
    /** directory 以下の全アセットの Guid を集める */
    void CollectAssetGuids(NanamiEngine::Core::FileSystem::Directory& directory, std::vector<::Guid>& outGuids)
    {
        for (auto& file : directory.Files())
        {
            if (file.GetContent())
                outGuids.push_back(file.GetContent()->GetGuid());
        }

        for (auto& child : directory.GetDirectories())
        {
            CollectAssetGuids(child, outGuids);
        }
    }
}

namespace NanamiEngine::Core::Application
{
    std::optional<Physics>             ApplicationBase::physics_         = std::optional<Core::Physics>();
    std::optional<FileSystem::Directory> ApplicationBase::assetsDirectory_ = std::nullopt;
    
    ApplicationBase::ApplicationBase()
    {
        /** ApplicationConfiguの初期化 */
        Configuration::AppConfiguration::Load();
        Configuration::NetworkConfiguration::Load();
        Configuration::PhysicsConfiguration::Load();
        SetDoubleStartValidFlag(true          );
        ChangeWindowMode       (true          );
        SetGraphMode           (Configuration::AppConfiguration::GetWindowWidth(), Configuration::AppConfiguration::GetWindowHeight(), Configuration::AppConfiguration::GetWindowColorScale());
        SetUseDirect3DVersion  (DX_DIRECT3D_11);
        // カメラが障害物に寄った際にNearクリップを動的に縮めるため、既定の16bitでは遠景のZ精度が足りない
        SetZBufferBitDepth     (Configuration::AppConfiguration::GetZBufferBitDepth());
        SetUseZBuffer3D        (TRUE          );
        SetWriteZBuffer3D      (TRUE          );
        SetDrawScreen          (DX_SCREEN_BACK);
        SetUseIMEFlag          (TRUE          );
        DxLib_Init             (              );

        /** リソースの初期化 */
        SetUseASyncLoadFlag(true);
        assetsDirectory_.emplace(Configuration::AppConfiguration::GetAssetsDirectoryPath());

        /** Windowの初期化 */
        SetUseSetDrawScreenSettingReset(false);
        MainWindows_().MakeWindow<MainWindow::GameWindow>();
        OnChangeWindow<MainWindow::GameWindow>();

        /** Sceneの初期化 */
        const auto initScene = std::make_shared<Scene::Scene>("Assets/Scene/GameManage.scene");
        MainWindows().Catch<MainWindow::GameWindow>()->AddContent     (initScene);
        MainWindows().Catch<MainWindow::GameWindow>()->ChangeMainScene(initScene);

        physics_.emplace();
        Physics().Initialize();

        /** Effekseerの初期化 */
        Effekseer_Init(Configuration::AppConfiguration::GetParticleMax());
        SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
        Effekseer_SetGraphicsDeviceLostCallbackFunctions();
    }

    void ApplicationBase::Run()
    {
        while (ProcessMessage() >= 0)
        {
            ClearDrawScreen();
            Time::Update();
            OnFrame();
            ScreenFlip();
        }
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
        return assetsDirectory_.value();
    }

    void ApplicationBase::ResetAssetsDirectory()
    {
        std::vector<::Guid> oldAssetGuids;
        if (assetsDirectory_)
            CollectAssetGuids(assetsDirectory_.value(), oldAssetGuids);

        // emplace で古いアセットが破棄されてから新しいアセットが登録される
        assetsDirectory_.emplace(Configuration::AppConfiguration::GetAssetsDirectoryPath());

        // .meta のあるアセットは同じ Guid で上書きされているので残り、.meta の無いアセット（毎回新しい Guid）の古い登録だけが消える
        for (const auto& guid : oldAssetGuids)
        {
            ObjectRegistry_().RemoveIfExpired(guid);
        }
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

    Network::PrefabObjectRegistry& ApplicationBase::NetworkPrefabObjectRegistry()
    {
        static Network::PrefabObjectRegistry networkPrefabRegistry;
        return networkPrefabRegistry;
    }

    Physics& ApplicationBase::Physics()
    {
        return physics_.value();
    }

    void ApplicationBase::ResetPhysics()
    {
        physics_.emplace();
        physics_->Initialize();
    }

    std::shared_ptr<MainWindow::GameWindow> ApplicationBase::GameWindow()
    {
        static std::shared_ptr<MainWindow::GameWindow> gameWindow = MainWindows().Catch<MainWindow::GameWindow>();;
        return gameWindow;
    }
}