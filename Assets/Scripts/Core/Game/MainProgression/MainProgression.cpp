#include "MainProgression.h"

#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace GameCore
{
    void SaveGameProgression(const GameProgresion& progression)
    {
        NanamiEngine::Module::LocalPrefs::SaveWithPath(
            PROGRESSION_SAVE_FILE_PATH,
            PROGRESSION_SAVE_FILE_KEY,
            progression);
    }

    GameProgresion LoadGameProgression()
    {
        const auto progression = NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<GameProgresion>(
            PROGRESSION_SAVE_FILE_PATH,
            PROGRESSION_SAVE_FILE_KEY,
            GameProgresion::FirstTouchDownMainIsLand);

        // NOTE: 旧セーブはステージ (GrassLandStage = 2) を持っていることがある。島以外からは再開しない
        if (progression != GameProgresion::FirstTouchDownMainIsLand)
            return GameProgresion::MainIsland;
        return progression;
    }

    REGISTER_LOCAL_PREF_WITH_PATH(
        GameProgresion,
        PROGRESSION_SAVE_FILE_KEY,
        GameProgresion::FirstTouchDownMainIsLand,
        PROGRESSION_SAVE_FILE_PATH)
}
