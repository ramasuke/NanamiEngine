#include "MainProgression.h"

#include "../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

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
}
