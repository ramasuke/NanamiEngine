#include "GameApplication.h"

#include <DxLib.h>
#include <string>

#include "../LifeCycle/ApplicationLifeCycle.h"
#include "../Window/Main/Game/GameWindow.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Module/SafeExecute/Engine_Module_SafeExecute.h"

namespace NanamiEngine::Core::Application::Game
{
    GameApplication::GameApplication()
    {
        // NOTE: ゲーム側のカーソル (GamePlay::Ui::GameCursor) を描くので OS のカーソルは消す
        SetMouseDispFlag(FALSE);
        GameWindow()->Play();
    }

    void GameApplication::OnFrame()
    {
        std::string frameErrorMessage;
        if (!Module::SafeExecutor::Execute([this]()
            {
                ApplicationLifeCycle_().OnUpdate();
                GetMainWindow()->OnUpdate();
            }, frameErrorMessage))
        {
            Module::LogError("[Frame] " + frameErrorMessage);
        }

        RenderVertex();
    }

    void GameApplication::OnExit()
    {
    }
}
