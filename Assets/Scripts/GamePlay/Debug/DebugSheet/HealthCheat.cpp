#include "HealthCheat.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"

namespace GamePlay::Debug
{
    bool HealthCheat::isKeepFullHealth_ = false;

    void HealthCheat::Update()
    {
        if (!isKeepFullHealth_)
            return;

        const auto avatar = GameCore::PlayerAvatar::Owner();
        if (!avatar)
            return;

        auto& status = avatar->PlayerStatus();
        if (status.IsDeath() || status.Health() >= status.MaxHealth())
            return;

        status.RestoreFullHealth();
    }
}
#endif
