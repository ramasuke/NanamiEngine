#include "ApplicationConfiguration_GameWindow.h"

#include <cstring>
#include <string>
#include <utility>

#include "../ApplicationConfiguration.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"
#include "ImGuiHelper.h"

namespace NanamiEngine::Core::Application::Configuration
{
    using Module::GameObject::GameObjectMark;
    using Module::GameObject::GameObjectMarkShape;

    constexpr auto DEFAULT_SHOW_MARKS              = true;
    constexpr auto DEFAULT_SHOW_MARKS_IN_PLAY_MODE = false;
    constexpr auto DEFAULT_SHOW_MARK               = true;

    bool GameWindowConfiguration::showMarks_           = DEFAULT_SHOW_MARKS;
    bool GameWindowConfiguration::showMarksInPlayMode_ = DEFAULT_SHOW_MARKS_IN_PLAY_MODE;
    GameWindowConfiguration::MarkFlags GameWindowConfiguration::showMarkFlags_ = []
    {
        MarkFlags flags;
        flags.fill(DEFAULT_SHOW_MARK);
        return flags;
    }();

    constexpr auto GAME_WINDOW_CONFIG_PATH                = "GameWindow/";
    constexpr auto GAME_WINDOW_SHOW_MARKS_KEY             = "ShowMarks";
    constexpr auto GAME_WINDOW_SHOW_MARKS_IN_PLAY_MODE_KEY = "ShowMarksInPlayMode";
    constexpr auto GAME_WINDOW_SHOW_MARK_KEY_PREFIX       = "ShowMark_";

    constexpr std::pair<GameObjectMarkShape, const char*> MARK_SHAPE_GROUPS[] = {
        { GameObjectMarkShape::Label,   "Label"   },
        { GameObjectMarkShape::Circle,  "Circle"  },
        { GameObjectMarkShape::Diamond, "Diamond" },
    };

    void GameWindowConfiguration::Load()
    {
        showMarks_           = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(GAME_WINDOW_CONFIG_PATH, GAME_WINDOW_SHOW_MARKS_KEY,              DEFAULT_SHOW_MARKS);
        showMarksInPlayMode_ = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(GAME_WINDOW_CONFIG_PATH, GAME_WINDOW_SHOW_MARKS_IN_PLAY_MODE_KEY, DEFAULT_SHOW_MARKS_IN_PLAY_MODE);

        for (size_t i = 0; i < showMarkFlags_.size(); ++i)
        {
            const std::string key = std::string(GAME_WINDOW_SHOW_MARK_KEY_PREFIX) + Module::GameObject::GAMEOBJECT_MARK_NAMES[i];
            showMarkFlags_[i] = Module::ProjectConfig::LoadOrDefaultWithPath<bool>(GAME_WINDOW_CONFIG_PATH, key, DEFAULT_SHOW_MARK);
        }
    }

    void GameWindowConfiguration::Save()
    {
        Module::ProjectConfig::SaveWithPath<bool>(GAME_WINDOW_CONFIG_PATH, GAME_WINDOW_SHOW_MARKS_KEY,              showMarks_);
        Module::ProjectConfig::SaveWithPath<bool>(GAME_WINDOW_CONFIG_PATH, GAME_WINDOW_SHOW_MARKS_IN_PLAY_MODE_KEY, showMarksInPlayMode_);

        for (size_t i = 0; i < showMarkFlags_.size(); ++i)
        {
            const std::string key = std::string(GAME_WINDOW_SHOW_MARK_KEY_PREFIX) + Module::GameObject::GAMEOBJECT_MARK_NAMES[i];
            Module::ProjectConfig::SaveWithPath<bool>(GAME_WINDOW_CONFIG_PATH, key, showMarkFlags_[i]);
        }
    }

    bool GameWindowConfiguration::ShouldDrawMark(const GameObjectMark mark, const bool isPlayMode)
    {
        if (APPLICATION_MODE != ApplicationMode::Editor)
            return false;

        if (!showMarks_ || (isPlayMode && !showMarksInPlayMode_))
            return false;

        if (Module::GameObject::ToShape(mark) == GameObjectMarkShape::None)
            return false;

        return showMarkFlags_[static_cast<size_t>(mark)];
    }

    void GameWindowConfiguration::DrawConfigGUI()
    {
        ImGui::Text("Mark");
        ImGui::Separator();

        bool changed = false;

        if (ImGui::Checkbox("Show Marks", &showMarks_))
            changed = true;

        ImGui::BeginDisabled(!showMarks_);

        if (ImGui::Checkbox("Show In Play Mode", &showMarksInPlayMode_))
            changed = true;

        for (const auto& [shape, groupName] : MARK_SHAPE_GROUPS)
        {
            ImGui::Spacing();
            ImGui::Text("%s", groupName);
            if (!ImGui::BeginTable(groupName, 4, ImGuiTableFlags_SizingFixedFit))
                continue;

            for (size_t i = 0; i < showMarkFlags_.size(); ++i)
            {
                const GameObjectMark mark = Module::GameObject::ToMark(static_cast<int>(i));
                if (Module::GameObject::ToShape(mark) != shape)
                    continue;

                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i));
                Module::GameObject::DrawMarkPreviewGui(mark);
                ImGui::SameLine();
                // "LabelRed" → "Red"
                const char* colorName = Module::GameObject::ToName(mark) + std::strlen(groupName);
                if (ImGui::Checkbox(colorName, &showMarkFlags_[i]))
                    changed = true;
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::TextDisabled("* Set a GameObject's mark in the Inspector");

        if (changed)
            Save();
    }
}
