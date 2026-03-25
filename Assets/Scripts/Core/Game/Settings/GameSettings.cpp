#include "GameSettings.h"

GameCore::GameSettings& GameCore::GameSettings::GetInstance()
{
    static GameSettings instance;
    return instance;
}
