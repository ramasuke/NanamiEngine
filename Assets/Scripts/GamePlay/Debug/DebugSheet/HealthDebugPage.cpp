#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <format>
#include <string>

#include "HealthCheat.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"
#include "../../../Core/Game/StatusParameter/Stamina/Stamina.h"

namespace GamePlay::Debug
{
    namespace
    {
        const char* ToStateLabel(const GameCore::PlayerAvatar::IPlayerAvatarStatus& status)
        {
            if (status.IsDeath())   return "死亡";
            if (status.IsDowned())  return "ダウン";
            if (status.IsInjured()) return "負傷";
            return "通常";
        }

        void DrawHealth()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            Widgets::Toggle("体力を減らさない", HealthCheat::IsKeepFullHealth());
            Widgets::Note("減った体力を毎フレーム最大まで戻す。体力を 0 にする一撃は防げない。");

            const auto avatar = GameCore::PlayerAvatar::Owner();
            if (!avatar)
            {
                Widgets::Note("プレイヤーがいない。ステージに入ってから使う。");
                return;
            }

            auto& status = avatar->PlayerStatus();
            Widgets::Header("今の状態");
            Widgets::Label("体力", std::format("{} / {}", status.Health().Value(), status.MaxHealth().Value()));
            Widgets::Label("スタミナ", std::format("{:.1f} / {:.1f}", status.Stamina().CurrentValue().Value(), status.MaxStamina().Value()));
            Widgets::Label("状態", ToStateLabel(status));

            Widgets::Header("回復");
            if (Widgets::Button("体力を全回復"))
                status.RestoreFullHealth();
            // NOTE: WakeUpPlayerRpc と同じ手順。倒れていないときの Revive は体力を復活時の割合まで下げてしまう
            if (!status.IsDowned())
            {
                Widgets::Note("ダウン中だけ起こせる。");
                return;
            }
            if (Widgets::Button("起こす"))
            {
                status.Revive();
                avatar->GetEventSceneStateMachine().OnChangeState(GameCore::PlayerAvatar::EventSceneStateType::GetUp);
            }
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(CheatHealth, "チート/体力", 31, GamePlay::Debug::DrawHealth)
#endif
