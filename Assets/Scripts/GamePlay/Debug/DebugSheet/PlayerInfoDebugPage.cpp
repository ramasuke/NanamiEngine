#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <format>
#include <string>

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Debug
{
    namespace
    {
        std::string FormatVec3(const glm::vec3& value)
        {
            return std::format("{:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z);
        }

        void DrawPlayerInfo()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            namespace Main    = GameCore::Scene::Main;

            const auto current = GameCore::Game::Instance().Scenes().CurrentSceneType();
            Widgets::Label("現在のシーン", current ? Main::ToString(*current) : "-");

            const auto avatar = GameCore::PlayerAvatar::Owner();
            if (!avatar)
            {
                Widgets::Note("プレイヤーがいない。");
                return;
            }

            Widgets::Label("職業", GameCore::PlayerAvatar::ToString(avatar->Type()));
            Widgets::Label("操作", avatar->IsAcceptingControl() ? "受け付け中" : "止まっている (会話・店など)");

            const auto& transform = avatar->PlayerTransform();
            const std::string position = FormatVec3(transform.GetWorldPos());
            const std::string rotation = FormatVec3(transform.GetWorldEulerAngle());

            Widgets::Header("位置");
            Widgets::Label("座標", position);
            Widgets::Label("回転 (度)", rotation);
            if (Widgets::Button("座標をコピー"))
                ImGui::SetClipboardText(position.c_str());
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(PlayerInfo, "情報/プレイヤー", 71, GamePlay::Debug::DrawPlayerInfo)
#endif
