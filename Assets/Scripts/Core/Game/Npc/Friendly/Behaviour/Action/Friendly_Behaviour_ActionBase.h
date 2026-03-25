#pragma once
#include <cstdint>

#include "../TickStatus/Friendly_Behaviour_TickStatus.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../cereal/include/cereal/types/polymorphic.hpp"
#include "TickContext/Friendly_Behaviour_TickContext.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::Npc::Friendly::Behaviour
{
    constexpr auto ANIMATOR_PARAM_NAME = "State";
    
    class ActionBase
    {
    public:
        virtual ~ActionBase() = default;
        TickStatus Tick(const Action::TickContext& context);
        void OnDrawGui();

    private:
        /** templateMethodパターン */
        virtual TickStatus DoTick(const Action::TickContext& context) = 0;
        virtual void DoDrawGui();

    protected:
        /** サンドボックスパターン */
        [[nodiscard]] std::shared_ptr<IPlayerAvatar> GetPlayerAvatar() const;
        
#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::ActionBase, 0);