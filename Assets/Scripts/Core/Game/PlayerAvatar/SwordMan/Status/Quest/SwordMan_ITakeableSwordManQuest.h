#pragma once
#include <cstdint>

#include "../rxcpp/rx.hpp"
#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore::PlayerAvatar
{
    enum class QuestType;
}

namespace GameCore::PlayerAvatar::Quest
{
    class CompletedQuestGroup;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    struct TickContext;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class IObservableStatusEvent;
}

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    class ITakeableSwordManQuest
    {
    public:
        virtual ~ITakeableSwordManQuest() = default;
        virtual void StartQuest(
            const PlayerAvatar::SwordMan::IObservableStatusEvent& event,
            PlayerAvatar::Quest::CompletedQuestGroup& completedQuestGroup) = 0;
        virtual void OnDrawGui() = 0;
        [[nodiscard]] virtual const PlayerAvatar::QuestType& QuestType() const = 0;
        
        template<class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template<class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(GameCore::Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest, 0)