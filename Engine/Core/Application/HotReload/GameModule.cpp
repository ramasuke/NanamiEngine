#include "GameModule.h"

#include <Windows.h>
#include <system_error>

#include "../ApplicationBase.h"
#include "../LifeCycle/ApplicationLifeCycle.h"
#include "../Window/Main/Game/GameWindow.h"
#include "../Window/Main/Factory/MainWindowFactory.h"
#include "../Window/Main/Group/MainWindowGroup.h"
#include "../Window/Popup/Factory/PopupWindowFactory.h"
#include "../Window/Popup/Group/PopupWindowGroup.h"
#include "../Window/Toolbar/Widget/EditorToolbarWidgetRegistry.h"
#include "../../Network/Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "../../Object/Registry/ObjectRegistry.h"
#include "../../../Module/Asset/Factory/AssetFactory.h"
#include "../../../Module/GameObject/ComponentGroup/AddComponenet/AddComponent.h"
#include "../../../Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../../../Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Module/Network/Rpc/Engine_Network_RpcHandlerRegistry.h"
#include "../../../Module/Serialization/Engine_Module_SerializationModuleUnloader.h"
#include "../../../Module/Serialization/Engine_Module_SharedStaticObject.h"
#include "../../../../Packages/DebugSheet/DebugSheetConfig.h"
#include "../../../../Packages/DebugSheet/Core/DebugSheet.h"

namespace NanamiEngine::Core::Application::HotReload
{
    namespace
    {
        constexpr const char* KEEP_OLD_MODULES_PREF_PATH = "HotReload/";
        constexpr const char* KEEP_OLD_MODULES_PREF_KEY  = "KeepOldModules";

        std::string PathToUtf8(const std::filesystem::path& path)
        {
            const std::u8string u8 = path.u8string();
            return std::string(u8.begin(), u8.end());
        }

        std::string LastErrorText()
        {
            return "GetLastError " + std::to_string(GetLastError());
        }
    }

    GameModule& GameModule::Instance()
    {
        static GameModule instance;
        return instance;
    }

    GameModule::GameModule()
    {
        keepOldModules_ = Module::LocalPrefs::LoadOrDefaultWithPath<bool>(KEEP_OLD_MODULES_PREF_PATH, KEEP_OLD_MODULES_PREF_KEY, true);
    }

    void GameModule::SetKeepOldModules(const bool keep)
    {
        keepOldModules_ = keep;
        Module::LocalPrefs::SaveWithPath<bool>(KEEP_OLD_MODULES_PREF_PATH, KEEP_OLD_MODULES_PREF_KEY, keep);
    }

    bool GameModule::LoadInitial(const std::filesystem::path& source, std::string& outError)
    {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(source, ec))
        {
            outError = "ゲーム DLL がありません: " + PathToUtf8(source);
            return false;
        }
        source_      = std::filesystem::absolute(source, ec);
        stagingRoot_ = source_.parent_path() / L"HotReload";
        ClearStagingRoot();
        return LoadGeneration(outError);
    }

    void GameModule::RequestReload()
    {
        reloadRequested_ = true;
    }

    void GameModule::OnFrameEnd()
    {
        if (!reloadRequested_)
            return;
        
        reloadRequested_ = false;
        if (!IsLoaded())
        {
            Module::LogError("HotReload: ゲーム DLL が読まれていないので差し替えられません");
            return;
        }
        Reload();
    }

    void GameModule::ClearStagingRoot() const
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(stagingRoot_, ec))
            return;
        // 読み込み中のフォルダは消せないので、消せるものだけ消す
        for (const auto& entry : std::filesystem::directory_iterator(stagingRoot_, ec))
            std::filesystem::remove_all(entry.path(), ec);
    }

    bool GameModule::LoadGeneration(std::string& outError)
    {
        std::error_code ec;
        const std::filesystem::path directory = stagingRoot_ / std::to_wstring(generation_ + 1);
        std::filesystem::create_directories(directory, ec);
        if (ec)
        {
            outError = "世代フォルダを作れません: " + PathToUtf8(directory) + " (" + ec.message() + ")";
            return false;
        }

        const std::filesystem::path dll = directory / source_.filename();
        std::filesystem::copy_file(source_, dll, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            outError = "ゲーム DLL をコピーできません: " + PathToUtf8(dll) + " (" + ec.message() + ")";
            return false;
        }
        // PDB は DLL に埋め込まれたファイル名で探されるので、同じ名前で隣に置く
        const std::filesystem::path sourcePdb = std::filesystem::path(source_).replace_extension(L".pdb");
        if (std::filesystem::is_regular_file(sourcePdb, ec))
            std::filesystem::copy_file(sourcePdb, directory / sourcePdb.filename(), std::filesystem::copy_options::overwrite_existing, ec);

        const HMODULE module = LoadLibraryExW(dll.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (module == nullptr)
        {
            outError = "ゲーム DLL を読めません: " + PathToUtf8(dll) + " (" + LastErrorText() + ")";
            return false;
        }
        current_ = module;
        ++generation_;
        Module::Log("HotReload: ゲーム DLL を読みました (世代 " + std::to_string(generation_) + "): " + PathToUtf8(dll));
        return true;
    }

    void GameModule::Reload()
    {
        void* const oldModule = current_;
        const auto  gameWindow = ApplicationBase::GameWindow();

        // 0. Play 中は状態を持ち越さない。
        const bool wasPlaying = gameWindow->IsPlayMode() || gameWindow->IsPlaying();
        
        //編集中なら開いているシーンを写す
        std::vector<MainWindow::GameWindow::SceneSnapshot> snapshots;
        if (!wasPlaying)
        {
            snapshots = gameWindow->TakeSceneSnapshots();
        }
        
        if (MainWindow::MainWindowGroup::IsWindowOfModule(ApplicationBase::GetMainWindow().get(), oldModule))
        {
            ApplicationBase::OnChangeWindow<MainWindow::GameWindow>();
        }

        // 1-3. Game.dll のコードを指すものを全部捨てる
        gameWindow->UnloadAllScenes();
        ApplicationBase::ReleaseAssetsDirectory();
        const std::size_t removedMainWindows  = ApplicationBase::MainWindows ().RemoveWindowsOfModule(oldModule);
        const std::size_t removedPopupWindows = ApplicationBase::PopupWindows().RemoveWindowsOfModule(oldModule);
        ApplicationBase::ApplicationLifeCycle().Clear();
        ApplicationBase::ObjectRegistry().PurgeExpired();
        ApplicationBase::NetworkPrefabObjectRegistry().PurgeExpired();

        // 4. 登録の解除
        std::size_t unregistered = 0;
        unregistered += Module::Asset::AssetFactory::Instance().UnregisterModule(oldModule);
        unregistered += MainWindow::MainWindowFactory::Instance().UnregisterModule(oldModule);
        unregistered += PopupWindow::PopupWindowFactory::Instance().UnregisterModule(oldModule);
        unregistered += Module::LocalPrefs::Editor::LocalPrefsRegistry::GetInstance().UnregisterModule(oldModule);
        unregistered += Module::GameObject::AddComponent::UnregisterModule(oldModule);
        unregistered += Toolbar::EditorToolbarWidgetRegistry::Instance().UnregisterModule(oldModule);
        unregistered += Module::Network::RpcHandlerRegistry::Instance().UnregisterModule(oldModule);
#if NANAMI_DEBUG_SHEET_ENABLED
        unregistered += DebugSheet::Sheet::Instance().UnregisterModule(oldModule);
#endif
        const auto serialization = Module::Serialization::SerializationModuleUnloader::Unregister(oldModule);

        // 5. 取り残しの確認。残っていれば FreeLibrary せず保険モードと同じ扱いにする
        const std::size_t leftoverObjects = ApplicationBase::ObjectRegistry().CountAliveOfModule(oldModule);
        const std::size_t leftoverCasters = Module::Serialization::SerializationModuleUnloader::CountLeftoverCasters(oldModule);
        const std::size_t leftoverStatics = Module::Serialization::SharedStaticObjects::CountOwnedBy(oldModule);
        const bool        hasLeftover     = leftoverObjects + leftoverCasters + leftoverStatics > 0;
        if (hasLeftover)
        {
            Module::LogError("HotReload: 古い DLL への参照が残っています (objects " + std::to_string(leftoverObjects)
                + ", casters " + std::to_string(leftoverCasters) + ", statics " + std::to_string(leftoverStatics) + ")。FreeLibrary はしません");
        }
        
        current_ = nullptr;
        if (keepOldModules_ || hasLeftover)
            retired_.push_back(oldModule);
        else if (!FreeLibrary(static_cast<HMODULE>(oldModule)))
            Module::LogError("HotReload: FreeLibrary に失敗しました (" + LastErrorText() + ")");
        Module::Serialization::SerializationModuleUnloader::ClearClassVersions();

        // 6. 新しい DLL
        std::string error;
        const bool loaded = LoadGeneration(error);
        if (!loaded)
            Module::LogError("HotReload: " + error);

        // 7. アセットとシーンを新しい型で作り直す
        ApplicationBase::ResetAssetsDirectory();
        gameWindow->RestoreScenes(snapshots);

        lastReport_ = std::string(loaded ? "reloaded" : "FAILED") + " gen " + std::to_string(generation_)
            + " | unregistered " + std::to_string(unregistered)
            + ", windows "       + std::to_string(removedMainWindows + removedPopupWindows)
            + ", cereal in "     + std::to_string(serialization.inputBindings) + " out " + std::to_string(serialization.outputBindings)
            + " casters "        + std::to_string(serialization.casters) + " (+" + std::to_string(serialization.sweptCasters) + " swept)"
            + ", statics "       + std::to_string(serialization.sharedStatics)
            + ", scenes "        + std::to_string(snapshots.size())
            + (keepOldModules_ || hasLeftover ? ", old DLL kept" : ", old DLL freed");
        Module::Log("HotReload: " + lastReport_);
    }
}
