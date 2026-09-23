#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/QuestBoardModel.h"

namespace GamePlay::Ui
{
    /** @brief 難度の点を rank 個だけ塗る。一覧の札と依頼書で同じものを使う */
    void ShowQuestBoardRankPips(
        const std::vector<FIELD(Component::ImageRenderer)>& pips,
        int rank,
        const std::shared_ptr<Asset::SpriteFile>& filledSprite,
        const std::shared_ptr<Asset::SpriteFile>& emptySprite);

    /** @brief 受注中・達成・準備中の判の絵。受付中は nullptr(判を押さない) */
    struct QuestBoardStampSprites
    {
        std::shared_ptr<Asset::SpriteFile> taking;
        std::shared_ptr<Asset::SpriteFile> cleared;
        std::shared_ptr<Asset::SpriteFile> preparing;

        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> For(QuestBoardState state) const;
    };

    /**
     * @brief 掲示板に貼った依頼書の札1枚。行は表示窓の分だけ作って使い回すので、中身は Bind のたびに差し替える。
     */
    class EventBoardQuestRow final : public Component::ComponentBase,
                                     public LifeCycleCallback::IAwakable
    {
    public:
        void Bind(const QuestBoardEntry& entry);
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        void SetHighlighted(bool isHighlighted);

    private:
        void OnAwake() override;
        /** @brief 生成直後に Bind が来ても困らないよう、自前の参照はここで揃える */
        void EnsureComponents();
        void RefreshAppearance() const;

        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) ticketRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) placeText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) rewardText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) eventChip_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) stateStamp_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) waxSeal_;
        [[serialize(0)]] std::vector<FIELD(Component::ImageRenderer)> rankPips_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) filledPipSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) emptyPipSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) takingStampSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) clearedStampSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) preparingStampSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) selectedTicketSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) unselectedTicketSprite_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hoverSound_;
        [[serialize(0)]] float selectedScale_ = 1.05f;

        glm::vec3 baseScale_ = glm::vec3(1.0f);
        bool isHighlighted_ = false;
        bool isHovering_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(placeText_));
            archive(CEREAL_NVP(rewardText_));
            archive(CEREAL_NVP(eventChip_));
            archive(CEREAL_NVP(stateStamp_));
            archive(CEREAL_NVP(waxSeal_));
            archive(CEREAL_NVP(rankPips_));
            archive(CEREAL_NVP(filledPipSprite_));
            archive(CEREAL_NVP(emptyPipSprite_));
            archive(CEREAL_NVP(takingStampSprite_));
            archive(CEREAL_NVP(clearedStampSprite_));
            archive(CEREAL_NVP(preparingStampSprite_));
            archive(CEREAL_NVP(selectedTicketSprite_));
            archive(CEREAL_NVP(unselectedTicketSprite_));
            archive(CEREAL_NVP(hoverSound_));
            archive(CEREAL_NVP(selectedScale_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(placeText_));
            if (version >= 0) archive(CEREAL_NVP(rewardText_));
            if (version >= 0) archive(CEREAL_NVP(eventChip_));
            if (version >= 0) archive(CEREAL_NVP(stateStamp_));
            if (version >= 0) archive(CEREAL_NVP(waxSeal_));
            if (version >= 0) archive(CEREAL_NVP(rankPips_));
            if (version >= 0) archive(CEREAL_NVP(filledPipSprite_));
            if (version >= 0) archive(CEREAL_NVP(emptyPipSprite_));
            if (version >= 0) archive(CEREAL_NVP(takingStampSprite_));
            if (version >= 0) archive(CEREAL_NVP(clearedStampSprite_));
            if (version >= 0) archive(CEREAL_NVP(preparingStampSprite_));
            if (version >= 0) archive(CEREAL_NVP(selectedTicketSprite_));
            if (version >= 0) archive(CEREAL_NVP(unselectedTicketSprite_));
            if (version >= 0) archive(CEREAL_NVP(hoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardQuestRow, 0);
