#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/RestorationBoardModel.h"
#include "../Row/Ui_EventBoard_RestorationRow.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の「復興」
     */
    class EventBoardRestorationPage final : public Component::ComponentBase
    {
    public:
        void BuildRows(size_t count);
        void SubscribeOnClickRow(std::function<void(size_t)> onClick) const;
        [[nodiscard]] size_t MaxVisibleRows() const { return static_cast<size_t>(maxVisibleRows_); }

        void Bind(const RestorationBoardModel& model) const;

    private:
        /** @param entry nullptr なら「まだ普請の段取りがない」を出す */
        void ShowDetail(const RestorationBoardEntry* entry, int balance) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 150.0f;
        [[serialize(0)]] int maxVisibleRows_ = 4;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreAboveMark_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreBelowMark_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) detailRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailNameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailStateText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailConditionText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailCostText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailBalanceText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailRemainText_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailDescriptionLines_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailSeal_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) emptyText_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) openSealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) lockedSealSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) restoredSealSprite_;
        [[serialize(0)]] Color32 defaultColor_ = Color32(48, 30, 20);
        [[serialize(0)]] Color32 fadedColor_   = Color32(104, 78, 54);
        [[serialize(0)]] Color32 refusedColor_ = Color32(146, 38, 30);
        [[serialize(0)]] Color32 remainColor_  = Color32(150, 84, 26);
        // NOTE: 残りの欄は筆のフォントで書く。本文のフォントには「―」が無い
        [[serialize(0)]] std::string notApplicableText_ = "―";
        [[serialize(0)]] std::string noConditionText_   = "なし";

        std::vector<std::weak_ptr<EventBoardRestorationRow>> rows_;

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
            archive(CEREAL_NVP(detailNameText_));
            archive(CEREAL_NVP(detailStateText_));
            archive(CEREAL_NVP(detailConditionText_));
            archive(CEREAL_NVP(detailCostText_));
            archive(CEREAL_NVP(detailBalanceText_));
            archive(CEREAL_NVP(detailRemainText_));
            archive(CEREAL_NVP(detailDescriptionLines_));
            archive(CEREAL_NVP(detailSeal_));
            archive(CEREAL_NVP(emptyText_));
            archive(CEREAL_NVP(openSealSprite_));
            archive(CEREAL_NVP(lockedSealSprite_));
            archive(CEREAL_NVP(restoredSealSprite_));
            archive(CEREAL_NVP(defaultColor_));
            archive(CEREAL_NVP(fadedColor_));
            archive(CEREAL_NVP(refusedColor_));
            archive(CEREAL_NVP(remainColor_));
            archive(CEREAL_NVP(notApplicableText_));
            archive(CEREAL_NVP(noConditionText_));
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
            if (version >= 0) archive(CEREAL_NVP(detailNameText_));
            if (version >= 0) archive(CEREAL_NVP(detailStateText_));
            if (version >= 0) archive(CEREAL_NVP(detailConditionText_));
            if (version >= 0) archive(CEREAL_NVP(detailCostText_));
            if (version >= 0) archive(CEREAL_NVP(detailBalanceText_));
            if (version >= 0) archive(CEREAL_NVP(detailRemainText_));
            if (version >= 0) archive(CEREAL_NVP(detailDescriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(detailSeal_));
            if (version >= 0) archive(CEREAL_NVP(emptyText_));
            if (version >= 0) archive(CEREAL_NVP(openSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(lockedSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(restoredSealSprite_));
            if (version >= 0) archive(CEREAL_NVP(defaultColor_));
            if (version >= 0) archive(CEREAL_NVP(fadedColor_));
            if (version >= 0) archive(CEREAL_NVP(refusedColor_));
            if (version >= 0) archive(CEREAL_NVP(remainColor_));
            if (version >= 0) archive(CEREAL_NVP(notApplicableText_));
            if (version >= 0) archive(CEREAL_NVP(noConditionText_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardRestorationPage, 0);
