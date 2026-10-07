#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <string>

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"

namespace GamePlay::Debug
{
    namespace
    {
        void DrawChangeScene()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            namespace Main    = GameCore::Scene::Main;

            const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
            if (!gameWindow || !gameWindow->IsPlaying())
            {
                Widgets::Note("プレイ中だけ使える。");
                return;
            }

            auto&      scenes  = GameCore::Game::Instance().Scenes();
            const auto current = scenes.CurrentSceneType();
            Widgets::Label("現在のシーン", current ? Main::ToString(*current) : "-");
            if (scenes.HasPendingChange())
            {
                
            }
                Widgets::Note("移動中...");

            Widgets::Header("移動先");
            for (const auto type : Main::SCENE_TYPES)
            {
                if (Widgets::Button(Main::ToString(type)))
                    scenes.RequestChangeScene(type);
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(SceneChange, "シーン/移動", 20, GamePlay::Debug::DrawChangeScene)
#endif
