#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <format>
#include <string>

#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"
#include "../../Network/Game_CustomNetworkRunner.h"
#include "../../Network/Relay/RelayServerSettings.h"
#include "Engine/Core/Network/Mode/NetworkSystem_ConnectionState.h"
#include "Engine/Core/Network/PlayerId/PlayerId.h"
#include "Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Debug
{
    namespace
    {
        const char* ToLabel(const NanamiEngine::Core::Network::ConnectionState state)
        {
            using NanamiEngine::Core::Network::ConnectionState;
            switch (state)
            {
            case ConnectionState::Connecting:   return "接続中";
            case ConnectionState::Connected:    return "接続済み";
            case ConnectionState::Failed:       return "失敗";
            case ConnectionState::Disconnected: return "切断";
            }
            return "-";
        }

        void DrawRunner()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            const auto* base = NanamiEngine::Module::Network::NetworkRunnerBase::TryGetInstance();
            if (!base)
            {
                Widgets::Note("NetworkRunner がいない。");
                return;
            }

            auto& runner = Network::CustomNetworkRunner::Instance();
            if (!runner.IsStarted())
            {
                Widgets::Label("状態", "未開始");
                return;
            }

            Widgets::Label("役割", runner.IsServer() ? "ホスト" : "クライアント");
            Widgets::Label("状態", ToLabel(runner.GetConnectionState()));
            Widgets::Label("PlayerId", runner.GetPlayerId().ToString());

            const std::string roomCode = runner.RelayRoomCode();
            Widgets::Label("部屋コード", roomCode.empty() ? "-" : roomCode);
            if (const auto failure = runner.RelayFailure())
                Widgets::Label("中継の失敗理由", *failure);
        }

        void DrawRelaySettings()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            // NOTE: 毎フレームファイルを読まないよう、初回と「読み直す」でだけ読む
            static auto relay = Network::RelayServerSettings::Load();
            Widgets::Label("中継サーバー", relay.IsEnabled() ? "有効" : "無効");
            Widgets::Label("接続先", std::format("{}:{}", relay.address, relay.port));
            Widgets::Label("appId", relay.appId);
            if (Widgets::Button("設定を読み直す"))
                relay = Network::RelayServerSettings::Load();
            Widgets::Note("変えるときはツールバー > LocalPrefs > RelayServer。");
        }

        void DrawAvatars()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            int count = 0;
            for (const auto& weakAvatar : GameCore::IPlayerAvatar::PlayerAvatars())
            {
                const auto avatar = weakAvatar.lock();
                if (!avatar)
                    continue;

                const auto position = avatar->PlayerTransform().GetWorldPos();
                const std::string key = std::string(GameCore::PlayerAvatar::ToString(avatar->Type())) + (avatar->IsOwner() ? " (自分)" : "");
                ImGui::PushID(avatar.get());
                Widgets::Label(key, std::format("{:.1f}, {:.1f}, {:.1f}", position.x, position.y, position.z));
                ImGui::PopID();
                ++count;
            }

            if (count == 0)
                Widgets::Note("なし");
        }

        void DrawNetworkInfo()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            Widgets::Header("接続");
            DrawRunner();

            Widgets::Header("中継サーバー設定");
            DrawRelaySettings();

            Widgets::Header("シーン内のアバター");
            DrawAvatars();
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(NetworkInfo, "情報/ネットワーク", 70, GamePlay::Debug::DrawNetworkInfo)
#endif
