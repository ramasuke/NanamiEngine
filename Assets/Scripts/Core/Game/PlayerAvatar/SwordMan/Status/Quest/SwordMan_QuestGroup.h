#pragma once
#include <memory>
#include <vector>
#include <cstdint>

#include "SwordMan_ITakeableSwordManQuest.h"
#include "../../../Quest/PlayerAvatar_IQuestGroup.h"
#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar::SwordMan
{
    class QuestGroup final : public IQuestGroup
    {
    public:
        explicit QuestGroup(
            const std::vector<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>>& quests = {});
        ~QuestGroup() override;

        void Init(const std::shared_ptr<IObservableStatusEvent>& event);
        void Subscribe(const std::shared_ptr<QuestBase>& addQuest) override;
        void Subscribe(const std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>& addQuest);
        void OnDrawGui();
        [[nodiscard]] const Quest::CompletedQuestGroup& Completed() const override { return *completedQuests_; }
        [[nodiscard]] std::unique_ptr<QuestGroup> DeepCoy() const;
        
    private:
        [[serialize(0)]] std::vector<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>> quests_;
        std::unique_ptr<Quest::CompletedQuestGroup> completedQuests_;
        std::shared_ptr<IObservableStatusEvent> event_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            std::size_t count = quests_.size();
            archive(CEREAL_NVP(count));

            for (const auto& quest : quests_)
            {
                archive(quest);  
            }
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            std::size_t count = 0;
            archive(CEREAL_NVP(count));

            quests_.clear();
            quests_.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
            {
                std::unique_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest> quest;
                archive(quest);
                quests_.emplace_back(std::move(quest));
            }
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::SwordMan::QuestGroup, 0)
