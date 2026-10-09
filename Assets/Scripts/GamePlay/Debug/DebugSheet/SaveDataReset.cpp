#include "SaveDataReset.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <algorithm>
#include <array>
#include <filesystem>
#include <string_view>

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Core/Game/PlayerAvatar/Record/PlayerAvatar_RecordBook.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"

namespace GamePlay::Debug
{
    bool SaveDataReset::isPending_ = false;

    namespace
    {
        // NOTE: 設定なので残す (WindowDisplayMode / RelayServerSettings / GameSettings)
        constexpr std::array<std::string_view, 3> KEPT_FOLDERS { "Display/", "Network/", "Settings/" };

        bool IsPlaying()
        {
            const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
            return gameWindow && gameWindow->IsPlaying();
        }

        // NOTE: アバターの居ない Title に落ち着いているか。プレイ中だけ呼ぶ
        bool IsSettledOnTitle()
        {
            const auto& scenes = GameCore::Game::Instance().Scenes();
            return scenes.CurrentSceneType() == GameCore::Scene::Main::SceneType::Title && !scenes.HasPendingChange();
        }
    }

    void SaveDataReset::Request()
    {
        if (!IsPlaying() || IsSettledOnTitle())
        {
            ResetNow();
            return;
        }

        isPending_ = true;
        GameCore::Game::Instance().Scenes().RequestChangeScene(GameCore::Scene::Main::SceneType::Title);
    }

    void SaveDataReset::Update()
    {
        if (!isPending_ || !IsSettledOnTitle())
            return;

        ResetNow();
    }

    std::vector<std::string> SaveDataReset::TargetFilePaths()
    {
        namespace fs = std::filesystem;

        const fs::path root = NanamiEngine::Module::LocalPrefs::LOCAL_PREFS_DATA_FOLDER_PATH;
        std::vector<std::string> paths;
        std::error_code error;
        if (!fs::exists(root, error))
            return paths;

        for (const auto& entry : fs::recursive_directory_iterator(root, error))
        {
            if (!entry.is_regular_file() || entry.path().extension() != NanamiEngine::Module::LocalPrefs::LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL)
                continue;

            const std::string relative = entry.path().lexically_relative(root).generic_string();
            const bool isKept = std::ranges::any_of(KEPT_FOLDERS, [&](const std::string_view folder) { return relative.starts_with(folder); });
            if (!isKept)
                paths.push_back((root / relative).generic_string());
        }

        std::ranges::sort(paths);
        return paths;
    }

    void SaveDataReset::ResetNow()
    {
        isPending_ = false;

        // NOTE: MainProgression も消えて序章に戻るので、StoryProgress::Reload が序章クリアを引き継ぐことはない
        for (const auto& path : TargetFilePaths())
        {
            std::error_code error;
            std::filesystem::remove(path, error);
            if (error)
                NanamiEngine::Module::LogWarning("SaveDataReset: " + path + " を消せませんでした: " + error.message());
        }

        // NOTE: SelectedPlayerAvatarType::Load は既定値を持たないので、初期の職業を書いておく
        GameCore::PlayerAvatar::SelectedPlayerAvatarType::Save(GameCore::PlayerAvatar::PlayerAvatarType::SwordMan);

        GameCore::Story::StoryProgress::Instance().Reload();
        GameCore::Decoration::DecorationCollection::Instance().Reload();
        GameCore::PlayerAvatar::Quest::QuestJournal::Instance().Reload();
        GameCore::PlayerAvatar::Record::RecordBook::Instance().Reload();

        NanamiEngine::Module::Log("SaveDataReset: 進行系のセーブを初期化しました");
    }
}
#endif
