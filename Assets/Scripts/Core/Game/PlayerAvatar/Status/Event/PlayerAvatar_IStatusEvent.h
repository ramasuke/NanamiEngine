#pragma once
#include "Packages/R4/R4.h"

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore::PlayerAvatar
{
    class IStatusEvent
    {
    public:
        virtual ~IStatusEvent() = default;

        [[nodiscard]] virtual NanamiEngine::R4::Observable<StatusParameter::Health> OnDamage() const = 0;
    };
}
