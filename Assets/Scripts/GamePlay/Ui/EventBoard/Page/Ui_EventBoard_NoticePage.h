#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/NoticeBoardModel.h"
#include "../Row/Ui_EventBoard_NoticeRow.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の「お知らせ」の頁。左にお知らせの札を並べ、右の便箋に選んだお知らせの本文を出す。
     */
    class EventBoardNoticePage final : public Component::ComponentBase
    {
    public:
        void BuildRows(size_t count);
        void SubscribeOnClickRow(std::function<void(size_t)> onClick) const;
        [[nodiscard]] size_t MaxVisibleRows() const { return static_cast<size_t>(maxVisibleRows_); }

        void Bind(const NoticeBoardModel& model) const;

    private:
        /** @param entry nullptr なら「お知らせなし」を出す */
        void ShowDetail(const NoticeBoardEntry* entry) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 150.0f;
        [[serialize(0)]] int maxVisibleRows_ = 4;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreAboveMark_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) moreBelowMark_;

        [[serialize(0)]] FIELD(GameObject::IGameObject) detailRoot_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) detailKindHanko_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailDateText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailTitleText_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailBodyLines_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) emptyText_;
        /** AnnouncementKind の順 */
        [[serialize(0)]] std::vector<FIELD(Asset::SpriteFile)> kindHankoSprites_;

        std::vector<std::weak_ptr<EventBoardNoticeRow>> rows_;

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
            archive(CEREAL_NVP(detailKindHanko_));
            archive(CEREAL_NVP(detailDateText_));
            archive(CEREAL_NVP(detailTitleText_));
            archive(CEREAL_NVP(detailBodyLines_));
            archive(CEREAL_NVP(emptyText_));
            archive(CEREAL_NVP(kindHankoSprites_));
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
            if (version >= 0) archive(CEREAL_NVP(detailKindHanko_));
            if (version >= 0) archive(CEREAL_NVP(detailDateText_));
            if (version >= 0) archive(CEREAL_NVP(detailTitleText_));
            if (version >= 0) archive(CEREAL_NVP(detailBodyLines_));
            if (version >= 0) archive(CEREAL_NVP(emptyText_));
            if (version >= 0) archive(CEREAL_NVP(kindHankoSprites_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardNoticePage, 0)
