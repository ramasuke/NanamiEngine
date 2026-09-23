#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/EventBoardModel.h"
#include "../Row/Ui_EventBoard_Row.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の「催し」の頁。左に告知の札を並べ、右のポスターに選んだ告知を出す。
     */
    class EventBoardEventPage final : public Component::ComponentBase
    {
    public:
        void BuildRows(size_t count);
        void SubscribeOnClickRow(std::function<void(size_t)> onClick) const;
        [[nodiscard]] size_t MaxVisibleRows() const { return static_cast<size_t>(maxVisibleRows_); }

        void Bind(const EventBoardModel& model) const;

    private:
        /** @param entry nullptr なら「予定なし」を出す */
        void ShowDetail(const EventBoardEntry* entry) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 150.0f;
        [[serialize(0)]] int maxVisibleRows_ = 4;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreAboveMark_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreBelowMark_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) detailRoot_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailBanner_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailTitleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailTagText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailPeriodText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailStatusText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailOngoingStamp_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailDescriptionLines_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) emptyText_;
        [[serialize(0)]] Color32 ongoingStatusColor_  = Color32(146, 38, 30);
        [[serialize(0)]] Color32 upcomingStatusColor_ = Color32(48, 30, 20);

        std::vector<std::weak_ptr<EventBoardRow>> rows_;

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
            archive(CEREAL_NVP(detailBanner_));
            archive(CEREAL_NVP(detailTitleText_));
            archive(CEREAL_NVP(detailTagText_));
            archive(CEREAL_NVP(detailPeriodText_));
            archive(CEREAL_NVP(detailStatusText_));
            archive(CEREAL_NVP(detailOngoingStamp_));
            archive(CEREAL_NVP(detailDescriptionLines_));
            archive(CEREAL_NVP(emptyText_));
            archive(CEREAL_NVP(ongoingStatusColor_));
            archive(CEREAL_NVP(upcomingStatusColor_));
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
            if (version >= 0) archive(CEREAL_NVP(detailBanner_));
            if (version >= 0) archive(CEREAL_NVP(detailTitleText_));
            if (version >= 0) archive(CEREAL_NVP(detailTagText_));
            if (version >= 0) archive(CEREAL_NVP(detailPeriodText_));
            if (version >= 0) archive(CEREAL_NVP(detailStatusText_));
            if (version >= 0) archive(CEREAL_NVP(detailOngoingStamp_));
            if (version >= 0) archive(CEREAL_NVP(detailDescriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(emptyText_));
            if (version >= 0) archive(CEREAL_NVP(ongoingStatusColor_));
            if (version >= 0) archive(CEREAL_NVP(upcomingStatusColor_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardEventPage, 0);
