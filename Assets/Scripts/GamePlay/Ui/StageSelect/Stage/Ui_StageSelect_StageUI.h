#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Scene/SceneFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/Component/ImageRenderer/Animation/ImageAnimationRenderer.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../Core/Game/Scene/Main/Type/MainSceneType.h"
#include "../../../../../../Assets/Data/Stage/Data_StageData.h"
#include "../Difficulty/StageDifficultyPips.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class StageSelectStageUi final : public Component::ComponentBase,
                                     public LifeCycleCallback::IAwakable
    {
    public:
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        [[nodiscard]] GameCore::Scene::Main::SceneType SceneType         () const { return stageData_->SceneType();          }
        [[nodiscard]] const std::string&               DisplayName       () const { return stageData_->DisplayName();        }
        [[nodiscard]] const glm::vec2&                 MapMarkerPosition () const { return stageData_->MapMarkerPosition();  }
        [[nodiscard]] bool                              IsCleared        () const { return stageData_->IsCleared();          }
        [[nodiscard]] std::shared_ptr<Asset::StageData> Data             () const { return stageData_.get();                 }

        [[nodiscard]] bool                              IsLocked         () const { return isLocked_;                        }

        void SetHighlighted(bool isHighlighted);
        /** @brief ロック中は名前を伏せ、属性アイコンを錠前にして難易度を隠す */
        void SetLocked(bool isLocked);

    private:
        void OnAwake() override;
        void RefreshAppearance();

    private:
        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) imageRenderer_;
        FIELD(NanamiUi::ImageAnimationRenderer) glowAnimation_;
        [[serialize(3)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(3)]] FIELD(Component::ImageRenderer) elementIcon_;
        [[serialize(3)]] FIELD(StageDifficultyPips) difficultyPips_;
        [[serialize(0)]] FIELD(Asset::SoundFile) selectButtonHoverSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) selectButtonClickSound_;
        [[serialize(1)]] FIELD(Asset::StageData) stageData_;
        [[serialize(1)]] FIELD(Asset::SpriteFile) selectedRowSprite_;
        [[serialize(1)]] FIELD(Asset::SpriteFile) unselectedRowSprite_;
        [[serialize(4)]] FIELD(Asset::UiSoundBankData) uiSounds_;
        [[serialize(5)]] FIELD(Asset::SpriteFile) lockedRowSprite_;
        [[serialize(5)]] FIELD(Asset::SpriteFile) lockedElementSprite_;
        bool isHighlighted_ = false;
        bool isLocked_ = false;
        bool isHovering_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(selectButtonHoverSound_));
            archive(CEREAL_NVP(selectButtonClickSound_));
            archive(CEREAL_NVP(stageData_));
            archive(CEREAL_NVP(selectedRowSprite_));
            archive(CEREAL_NVP(unselectedRowSprite_));
            archive(CEREAL_NVP(elementIcon_));
            archive(CEREAL_NVP(difficultyPips_));
            archive(CEREAL_NVP(uiSounds_));
            archive(CEREAL_NVP(lockedRowSprite_));
            archive(CEREAL_NVP(lockedElementSprite_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 3) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(selectButtonHoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectButtonClickSound_));
            if (version >= 1) archive(CEREAL_NVP(stageData_));
            if (version >= 1) archive(CEREAL_NVP(selectedRowSprite_));
            if (version >= 1) archive(CEREAL_NVP(unselectedRowSprite_));
            if (version >= 3) archive(CEREAL_NVP(elementIcon_));
            if (version >= 3) archive(CEREAL_NVP(difficultyPips_));
            if (version >= 4) archive(CEREAL_NVP(uiSounds_));
            if (version >= 5) archive(CEREAL_NVP(lockedRowSprite_));
            if (version >= 5) archive(CEREAL_NVP(lockedElementSprite_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectStageUi, 5);
