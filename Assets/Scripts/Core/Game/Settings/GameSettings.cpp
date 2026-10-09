#include "GameSettings.h"

#include <algorithm>
#include <array>

#include "Engine/Module/Audio/AudioVolume.h"

#include "Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore
{
    constexpr auto GAME_SETTINGS_FILE_KEY  = "GameSettings";
    constexpr auto GAME_SETTINGS_FILE_PATH = "Settings/";

    namespace
    {
        std::string_view ToString(const ChatAdvanceMode mode)
        {
            switch (mode)
            {
            case ChatAdvanceMode::Manual: return "Manual";
            case ChatAdvanceMode::Auto:   return "Auto";
            }
            return "?";
        }
    }

    GameSettings& GameSettings::GetInstance()
    {
        // NOTE: 初回呼び出し時に LocalPrefs からロードする。ファイルがない場合はデフォルト値を使用する
        static GameSettings instance = NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<GameSettings>(
            GAME_SETTINGS_FILE_PATH, GAME_SETTINGS_FILE_KEY, GameSettings{});
        static const bool applied = (instance.ApplyAudioVolume(), true);
        (void)applied;
        return instance;
    }

    void GameSettings::Save() const
    {
        NanamiEngine::Module::LocalPrefs::SaveWithPath(GAME_SETTINGS_FILE_PATH, GAME_SETTINGS_FILE_KEY, *this);
    }

    void GameSettings::SetMasterVolume(const int step)
    {
        masterVolume_ = std::clamp(step, 0, VOLUME_STEPS);
        ApplyAudioVolume();
    }

    void GameSettings::SetBgmVolume(const int step)
    {
        bgmVolume_ = std::clamp(step, 0, VOLUME_STEPS);
        ApplyAudioVolume();
    }

    void GameSettings::SetSeVolume(const int step)
    {
        seVolume_ = std::clamp(step, 0, VOLUME_STEPS);
        ApplyAudioVolume();
    }

    void GameSettings::ApplyAudioVolume() const
    {
        namespace Audio = NanamiEngine::Module::Audio;
        const auto to01 = [](const int step) { return static_cast<float>(std::clamp(step, 0, VOLUME_STEPS)) / VOLUME_STEPS; };
        Audio::SetMasterVolume(to01(masterVolume_));
        Audio::SetCategoryVolume(Audio::AudioCategory::Bgm, to01(bgmVolume_));
        Audio::SetCategoryVolume(Audio::AudioCategory::Se, to01(seVolume_));
    }

    void GameSettings::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("Chat Char Interval (secs)",      chatTextCharInterval_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("Chat Sentence Interval (secs)",  chatTextSentenceInterval_secs_);
        LibCore::ImGuiHelper::OnDrawEnumField("Chat Advance Mode", chatAdvanceMode_,
            std::array{ChatAdvanceMode::Manual, ChatAdvanceMode::Auto}, &ToString);
        ImGui::SliderInt("Master Volume", &masterVolume_, 0, VOLUME_STEPS);
        ImGui::SliderInt("BGM Volume",    &bgmVolume_,    0, VOLUME_STEPS);
        ImGui::SliderInt("SE Volume",     &seVolume_,     0, VOLUME_STEPS);
    }

    REGISTER_LOCAL_PREF_WITH_PATH(GameSettings, GAME_SETTINGS_FILE_KEY, GameSettings{}, GAME_SETTINGS_FILE_PATH)
}
