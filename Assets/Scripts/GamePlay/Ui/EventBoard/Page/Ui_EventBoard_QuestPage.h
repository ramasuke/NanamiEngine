#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/QuestBoardModel.h"
#include "../Row/Ui_EventBoard_QuestRow.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の「依頼」の頁。左に依頼書の札を並べ、右に選んだ依頼書を広げる。
     * 右下の受注印は、受付中なら点線の枠、受けたら朱の印に替わる。
     */
    class EventBoardQuestPage final : public Component::ComponentBase
    {
    public:
        void BuildRows(size_t count);
        void SubscribeOnClickRow(std::function<void(size_t)> onClick) const;
        [[nodiscard]] size_t MaxVisibleRows() const { return static_cast<size_t>(maxVisibleRows_); }

        void Bind(const QuestBoardModel& model) const;

    private:
        /** @param entry nullptr なら「依頼なし」を出す */
        void ShowDetail(const QuestBoardEntry* entry) const;
        /** @brief ステージのサムネイル (大きさはまちまち) を写真枠いっぱいに縮める */
        void FitPhotoToFrame(int photoHandle) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 150.0f;
        [[serialize(0)]] int maxVisibleRows_ = 4;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreAboveMark_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreBelowMark_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) detailRoot_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) detailPhotoRoot_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailPhoto_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailEventChip_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailEventText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailTitleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailClientText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailPlaceText_;
        [[serialize(0)]] std::vector<FIELD(Component::ImageRenderer)> detailRankPips_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailStateText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailGoalText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailRewardText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailLimitText_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailDescriptionLines_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailSeal_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) emptyText_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) filledPipSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) emptyPipSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) openSealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) takingSealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) clearedSealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) preparingSealSprite_;
        [[serialize(0)]] Color32 takingStateColor_  = Color32(146, 38, 30);
        [[serialize(0)]] Color32 defaultStateColor_ = Color32(48, 30, 20);
        [[serialize(1)]] FIELD(Asset::SpriteFile) lockedSealSprite_;
        /** @brief 写真枠の内側の大きさ。サムネイルはこれを覆う倍率に縮める */
        [[serialize(2)]] glm::vec2 detailPhotoSize_px_ = glm::vec2(400.0f, 240.0f);

        std::vector<std::weak_ptr<EventBoardQuestRow>> rows_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(rowsRoot_));
            archive(CEREAL_NVP(rowSpacing_px_));
            archive(CEREAL_NVP(maxVisibleRows_));
            archive(CEREAL_NVP(moreAboveMark_));
            archive(CEREAL_NVP(moreBelowMark_));
            archive(CEREAL_NVP(detailRoot_));
            archive(CEREAL_NVP(detailPhotoRoot_));
            archive(CEREAL_NVP(detailPhoto_));
            archive(CEREAL_NVP(detailEventChip_));
            archive(CEREAL_NVP(detailEventText_));
            archive(CEREAL_NVP(detailTitleText_));
            archive(CEREAL_NVP(detailClientText_));
            archive(CEREAL_NVP(detailPlaceText_));
            archive(CEREAL_NVP(detailRankPips_));
            archive(CEREAL_NVP(detailStateText_));
            archive(CEREAL_NVP(detailGoalText_));
            archive(CEREAL_NVP(detailRewardText_));
            archive(CEREAL_NVP(detailLimitText_));
            archive(CEREAL_NVP(detailDescriptionLines_));
            archive(CEREAL_NVP(detailSeal_));
            archive(CEREAL_NVP(emptyText_));
            archive(CEREAL_NVP(filledPipSprite_));
            archive(CEREAL_NVP(emptyPipSprite_));
            archive(CEREAL_NVP(openSealSprite_));
            archive(CEREAL_NVP(takingSealSprite_));
            archive(CEREAL_NVP(clearedSealSprite_));
            archive(CEREAL_NVP(preparingSealSprite_));
            archive(CEREAL_NVP(takingStateColor_));
            archive(CEREAL_NVP(defaultStateColor_));
            archive(CEREAL_NVP(lockedSealSprite_));
            archive(CEREAL_NVP(detailPhotoSize_px_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(rowsRoot_));
            if (version >= 0) archive(CEREAL_NVP(rowSpacing_px_));
            if (version >= 0) archive(CEREAL_NVP(maxVisibleRows_));
            if (version >= 0) archive(CEREAL_NVP(moreAboveMark_));
            if (version >= 0) archive(CEREAL_NVP(moreBelowMark_));
            if (version >= 0) archive(CEREAL_NVP(detailRoot_));
            if (version >= 0) archive(CEREAL_NVP(detailPhotoRoot_));
            if (version >= 0) archive(CEREAL_NVP(detailPhoto_));
            if (version >= 0) archive(CEREAL_NVP(detailEventChip_));
            if (version >= 0) archive(CEREAL_NVP(detailEventText_));
            if (version >= 0) archive(CEREAL_NVP(detailTitleText_));
            if (version >= 0) archive(CEREAL_NVP(detailClientText_));
            if (version >= 0) archive(CEREAL_NVP(detailPlaceText_));
            if (version >= 0) archive(CEREAL_NVP(detailRankPips_));
            if (version >= 0) archive(CEREAL_NVP(detailStateText_));
            if (version >= 0) archive(CEREAL_NVP(detailGoalText_));
            if (version >= 0) archive(CEREAL_NVP(detailRewardText_));
            if (version >= 0) archive(CEREAL_NVP(detailLimitText_));
            if (version >= 0) archive(CEREAL_NVP(detailDescriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(detailSeal_));
            if (version >= 0) archive(CEREAL_NVP(emptyText_));
            if (version >= 0) archive(CEREAL_NVP(filledPipSprite_));
            if (version >= 0) archive(CEREAL_NVP(emptyPipSprite_));
            if (version >= 0) archive(CEREAL_NVP(openSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(takingSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(clearedSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(preparingSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(takingStateColor_));
            if (version >= 0) archive(CEREAL_NVP(defaultStateColor_));
            if (version >= 1) archive(CEREAL_NVP(lockedSealSprite_));
            if (version >= 2) archive(CEREAL_NVP(detailPhotoSize_px_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardQuestPage, 2);
