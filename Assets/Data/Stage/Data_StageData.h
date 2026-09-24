#pragma once
#include <string>
#include <vector>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "vec2.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Quest/Unlock/PlayerAvatar_IQuestUnlockCondition.h"
#include "../../Scripts/Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto STAGE_DATA_EXTENSION_LABEL = ".stageData";

    /**
     * @brief ステージ選択の1行。unlockConditions_ を全部満たすまでは「？？？」で出し、
     * 選べても出発はできない(lockedDescriptionLines_ で条件を伝える)。
     */
    class StageData final : public ScriptableObject
    {
    public:
        explicit StageData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              DisplayName      () const { return displayName_;       }
        [[nodiscard]] GameCore::Scene::Main::SceneType SceneType        () const { return sceneType_;         }
        [[nodiscard]] const glm::vec2&                 MapMarkerPosition() const { return mapMarkerPosition_; }
        [[nodiscard]] bool                             IsCleared        () const { return isCleared_;        }
        [[nodiscard]] std::shared_ptr<SpriteFile>      ThumbnailSprite  () const { return thumbnailSprite_.get(); }
        [[nodiscard]] std::shared_ptr<SpriteFile>      ElementSprite    () const { return elementSprite_.get();   }
        [[nodiscard]] int                              Difficulty       () const { return difficulty_;       }
        [[nodiscard]] const std::string&               TagText          () const { return tagText_;          }
        [[nodiscard]] const std::vector<std::string>&  DescriptionLines () const { return descriptionLines_; }
        [[nodiscard]] const std::vector<std::string>&  LockedDescriptionLines() const { return lockedDescriptionLines_; }
        [[nodiscard]] bool IsUnlocked(const GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockContext& context) const;

    private:
        [[serialize(0)]] std::string                              displayName_;
        [[serialize(0)]] GameCore::Scene::Main::SceneType          sceneType_ = GameCore::Scene::Main::SceneType::GrassLand;
        [[serialize(1)]] glm::vec2                                mapMarkerPosition_ = glm::vec2(960.0f, 540.0f);
        [[serialize(1)]] bool                                     isCleared_ = false;
        [[serialize(2)]] FIELD(SpriteFile)                        thumbnailSprite_;
        [[serialize(2)]] FIELD(SpriteFile)                        elementSprite_;
        [[serialize(2)]] int                                      difficulty_ = 1;
        [[serialize(2)]] std::string                              tagText_;
        [[serialize(2)]] std::vector<std::string>                 descriptionLines_;
        [[serialize(3)]] GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockConditions unlockConditions_;
        [[serialize(3)]] std::vector<std::string>                 lockedDescriptionLines_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(sceneType_));
            archive(CEREAL_NVP(mapMarkerPosition_));
            archive(CEREAL_NVP(isCleared_));
            archive(CEREAL_NVP(thumbnailSprite_));
            archive(CEREAL_NVP(elementSprite_));
            archive(CEREAL_NVP(difficulty_));
            archive(CEREAL_NVP(tagText_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(unlockConditions_));
            archive(CEREAL_NVP(lockedDescriptionLines_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(sceneType_));
            if (version >= 1) archive(CEREAL_NVP(mapMarkerPosition_));
            if (version >= 1) archive(CEREAL_NVP(isCleared_));
            if (version >= 2) archive(CEREAL_NVP(thumbnailSprite_));
            if (version >= 2) archive(CEREAL_NVP(elementSprite_));
            if (version >= 2) archive(CEREAL_NVP(difficulty_));
            if (version >= 2) archive(CEREAL_NVP(tagText_));
            if (version >= 2) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 3) archive(CEREAL_NVP(unlockConditions_));
            if (version >= 3) archive(CEREAL_NVP(lockedDescriptionLines_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::StageData, 3);
#pragma endregion
