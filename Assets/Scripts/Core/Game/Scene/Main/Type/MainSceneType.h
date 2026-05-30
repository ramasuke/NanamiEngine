#pragma once
#include <array>
#include <string_view>

namespace GameCore::Scene::Main
{
    enum class SceneType : int
    {
        GrassLand = 0,
        FirstTouchDownMainIsLand = 1,
        MainIsland = 2,
        Title = 3,
    };

    constexpr std::array SCENE_TYPES
    {
        SceneType::GrassLand,
        SceneType::FirstTouchDownMainIsLand,
        SceneType::MainIsland,
        SceneType::Title,
    };

    constexpr std::string_view ToString(const SceneType type)
    {
        switch (type)
        {
        case SceneType::GrassLand: return "GrassLand";
        case SceneType::FirstTouchDownMainIsLand: return "FirstTouchDownMainIsLand";
        case SceneType::MainIsland: return "MainIsland";
        case SceneType::Title: return "Title";
        }

        return "Unknown";
    }
}
