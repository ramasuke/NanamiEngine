#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <array>
#include <string>

#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "../../Network/Relay/RelayProtocol.h"
#include "../../Network/Relay/RelayRoom.h"
#include "../../Network/Relay/RelayServerSettings.h"
#include "../../Network/Session/GamePlay_StageSessionMatchmaking.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"

namespace GamePlay::Debug
{
    namespace
    {
        using RelayRoom = Network::RelayRoom;

        // NOTE: StageMatchmaker::JoinOrHostAsync で部屋に入るステージだけ
        constexpr std::array STAGE_TYPES
        {
            GameCore::Scene::Main::SceneType::GrassLand,
            GameCore::Scene::Main::SceneType::Desert,
            GameCore::Scene::Main::SceneType::DragonNest,
        };

        const char* ToLabel(const RelayRoom::Mode mode)
        {
            switch (mode)
            {
            case RelayRoom::Mode::Public: return "公開";
            case RelayRoom::Mode::Create: return "作成";
            case RelayRoom::Mode::Join:   return "参加";
            }
            return "-";
        }

        /** 数字だけ・最大 ROOM_CODE_LENGTH 桁の部屋コード入力 */
        void InputRoomCode(std::string& code)
        {
            std::array<char, NanamiRelay::ROOM_CODE_LENGTH + 1> buffer{};
            code.copy(buffer.data(), NanamiRelay::ROOM_CODE_LENGTH);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputTextWithHint("##RoomCode", "部屋コード", buffer.data(), buffer.size(), ImGuiInputTextFlags_CharsDecimal))
                code = buffer.data();
        }

        void DrawStageEntry()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            namespace Main    = GameCore::Scene::Main;

            const auto gameWindow = NanamiEngine::Core::Application::ApplicationBase::GameWindow();
            if (!gameWindow || !gameWindow->IsPlaying())
            {
                Widgets::Note("プレイ中だけ使える。");
                return;
            }

            static RelayRoom::Mode mode = RelayRoom::Mode::Public;
            static std::string     code;

            auto&      game    = GameCore::Game::Instance();
            auto&      scenes  = game.Scenes();
            const auto current = scenes.CurrentSceneType();
            Widgets::Label("現在のシーン", current ? Main::ToString(*current) : "-");
            if (scenes.HasPendingChange())
                Widgets::Note("移動中...");

            const bool isRelayEnabled = Network::RelayServerSettings::Load().IsEnabled();
            Widgets::Label("中継サーバー", isRelayEnabled ? "有効" : "無効");

            Widgets::Header("接続方法");
            if (const int pressed = Widgets::ButtonRow("部屋", { ToLabel(RelayRoom::Mode::Public), ToLabel(RelayRoom::Mode::Create), ToLabel(RelayRoom::Mode::Join) }); pressed >= 0)
                mode = static_cast<RelayRoom::Mode>(pressed);
            Widgets::Label("選択中", ToLabel(mode));

            const RelayRoom room{ mode, mode == RelayRoom::Mode::Join ? code : std::string() };
            if (room.IsPrivate() && !isRelayEnabled)
                Widgets::Note("中継サーバーが無効なので、作成・参加では入れない。");
            if (mode == RelayRoom::Mode::Join)
                InputRoomCode(code);

            Widgets::Header("行き先");
            if (mode == RelayRoom::Mode::Join && code.size() != NanamiRelay::ROOM_CODE_LENGTH)
            {
                Widgets::Note("部屋コードを " + std::to_string(NanamiRelay::ROOM_CODE_LENGTH) + " 桁入れると出発できる。");
                return;
            }

            for (const auto type : STAGE_TYPES)
            {
                // NOTE: 移動中に押すと、先に予約した部屋を上書きしてしまう
                if (Widgets::Button(Main::ToString(type)) && !scenes.HasPendingChange())
                {
                    game.Matchmaker().SetNextRoom(room);
                    scenes.RequestChangeScene(type);
                }
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(StageEntry, "シーン/ステージへ出発", 21, GamePlay::Debug::DrawStageEntry)
#endif
