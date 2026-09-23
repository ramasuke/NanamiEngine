#include "EnemySpawnPoint.h"
#include "../../../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy
{
    bool EnemySpawnPoint::ShouldSpawn() const
    {
        if (skipIfStoryFlag_ < 0)
            return true;

        return !Story::StoryProgress::Instance().IsSet(
            static_cast<Story::StoryFlag>(skipIfStoryFlag_));
    }

    void EnemySpawnPoint::OnDrawGui()
    {
        ImGuiHelper::OnDrawEnumField("kind_", kind_, ENEMY_KINDS, ToString);

        bool hasSkipFlag = skipIfStoryFlag_ >= 0;
        if (ImGui::Checkbox("skipIfStoryFlag", &hasSkipFlag))
        {
            skipIfStoryFlag_ = hasSkipFlag ? 0 : -1;
        }
        if (hasSkipFlag)
        {
            auto flag = static_cast<Story::StoryFlag>(skipIfStoryFlag_);
            ImGuiHelper::OnDrawEnumField("skipIfStoryFlag_", flag, Story::STORY_FLAGS, Story::ToString);
            skipIfStoryFlag_ = static_cast<int>(flag);
        }
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Npc::Enemy::EnemySpawnPoint);
#pragma endregion
