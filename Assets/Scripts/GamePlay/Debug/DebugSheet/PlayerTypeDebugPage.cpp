#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../../Core/Game/Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"

namespace GamePlay::Debug
{
    namespace
    {
        void DrawPlayerType()
        {
            namespace Widgets      = NanamiEngine::DebugSheet::Widgets;
            namespace Main         = GameCore::Scene::Main;
            namespace PlayerAvatar = GameCore::PlayerAvatar;

            const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
            if (!gameWindow || !gameWindow->IsPlaying())
            {
                Widgets::Note("プレイ中だけ使える。");
                return;
            }

            const auto owner = PlayerAvatar::Owner();
            Widgets::Label("今の職業", owner ? PlayerAvatar::ToString(owner->Type()) : "-");

            // NOTE: ステージでは抜けるときに今の職業で保存し直されるので、予約もできない。拠点の切り替えだけ使う
            auto&      scenes = GameCore::Game::Instance().Scenes();
            const auto scene  = scenes.CurrentSceneType() == Main::SceneType::MainIsland && !scenes.HasPendingChange()
                ? scenes.Catch<Main::MainIslandScene>(Main::SceneType::MainIsland)
                : nullptr;
            if (!scene)
            {
                Widgets::Note("拠点 (MainIsland) でだけ切り替えられる。");
                return;
            }

            Widgets::Header("切り替え");
            for (const auto type : PlayerAvatar::PLAYER_AVATAR_TYPES)
            {
                if (Widgets::Button(PlayerAvatar::ToString(type)))
                    scene->SwitchPlayerAvatar(type);
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(PlayerType, "チート/職業", 41, GamePlay::Debug::DrawPlayerType)
#endif
