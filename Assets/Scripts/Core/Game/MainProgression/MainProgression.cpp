#include "MainProgression.h"

#include "../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../../../../../Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

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
        return NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath<GameProgresion>(
            PROGRESSION_SAVE_FILE_PATH,
            PROGRESSION_SAVE_FILE_KEY,
            GameProgresion::FirstTouchDownMainIsLand);
    }

    REGISTER_LOCAL_PREF_WITH_PATH(
        GameProgresion,
        PROGRESSION_SAVE_FILE_KEY,
        GameProgresion::FirstTouchDownMainIsLand,
        PROGRESSION_SAVE_FILE_PATH)
}
