#pragma once
#include "../TickStatus/TickStatus.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "TickContext/Enemy_Behaviour_TickContext.h"

namespace GameCore::Npc::Enemy::Behaviour
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
        //LifeCycleCallback::Update()で呼ばれる。
        virtual TickStatus DoTick(const Action::TickContext& context) = 0;
        virtual void DoDrawGui();
        
#pragma region Serialization Function
    public:
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::ActionBase, 0);
