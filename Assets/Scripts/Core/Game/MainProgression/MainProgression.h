#pragma once
namespace GameCore
{
    constexpr auto PROGRESSION_SAVE_FILE_PATH = "GameProgression/MainProgress";
    constexpr auto PROGRESSION_SAVE_FILE_KEY  = "Info";
    
    enum class GameProgresion : int
    {
        FirstTouchDownMainIsLand = 0,
        MainIsland = 1,
    };
    
    void SaveGameProgression(const GameProgresion& progression);
    GameProgresion LoadGameProgression();
}
