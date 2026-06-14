#include "GameSettings.h"

#include "../../../../../Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore
{
    constexpr auto GAME_SETTINGS_FILE_KEY  = "GameSettings";
    constexpr auto GAME_SETTINGS_FILE_PATH = "Settings/";

    GameSettings& GameSettings::GetInstance()
    {
        // 初回呼び出し時に LocalPrefs からロードする。ファイルがない場合はデフォルト値を使用する
        static GameSettings instance = NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<GameSettings>(
            GAME_SETTINGS_FILE_PATH, GAME_SETTINGS_FILE_KEY, GameSettings{});
        return instance;
    }

    void GameSettings::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("Chat Char Interval (secs)",      chatTextCharInterval_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("Chat Sentence Interval (secs)",  chatTextSentenceInterval_secs_);
    }

    REGISTER_LOCAL_PREF_WITH_PATH(GameSettings, GAME_SETTINGS_FILE_KEY, GameSettings{}, GAME_SETTINGS_FILE_PATH)
}
