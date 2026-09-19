#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "../../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/NoticeBoardModel.h"

namespace GamePlay::Ui
{
    /**
     * @brief 種類ごとの絵(札・角印)を AnnouncementKind の順に並べたものから引く。足りなければ nullptr
     */
    [[nodiscard]] std::shared_ptr<Asset::SpriteFile> FindAnnouncementKindSprite(
        const std::vector<FIELD(Asset::SpriteFile)>& spritesByKind,
        Asset::AnnouncementKind kind);

    /**
     * @brief 掲示板に貼ったお知らせの札1枚。行は表示窓の分だけ作って使い回すので、中身は Bind のたびに差し替える。
     */
    class EventBoardNoticeRow final : public Component::ComponentBase,
                                      public LifeCycleCallback::IAwakable
    {
    public:
        void Bind(const NoticeBoardEntry& entry);
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        void SetHighlighted(bool isHighlighted);

    private:
        void OnAwake() override;
        /** @brief 生成直後に Bind が来ても困らないよう、自前の参照はここで揃える */
        void EnsureComponents();
        void RefreshAppearance() const;

        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) ticketRenderer_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) kindChip_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) dateText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) unreadSeal_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) waxSeal_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) selectedTicketSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) unselectedTicketSprite_;
        /** AnnouncementKind の順 */
        [[serialize(0)]] std::vector<FIELD(Asset::SpriteFile)> kindChipSprites_;
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
            archive(CEREAL_NVP(kindChip_));
            archive(CEREAL_NVP(dateText_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(unreadSeal_));
            archive(CEREAL_NVP(waxSeal_));
            archive(CEREAL_NVP(selectedTicketSprite_));
            archive(CEREAL_NVP(unselectedTicketSprite_));
            archive(CEREAL_NVP(kindChipSprites_));
            archive(CEREAL_NVP(hoverSound_));
            archive(CEREAL_NVP(selectedScale_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(kindChip_));
            if (version >= 0) archive(CEREAL_NVP(dateText_));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(unreadSeal_));
            if (version >= 0) archive(CEREAL_NVP(waxSeal_));
            if (version >= 0) archive(CEREAL_NVP(selectedTicketSprite_));
            if (version >= 0) archive(CEREAL_NVP(unselectedTicketSprite_));
            if (version >= 0) archive(CEREAL_NVP(kindChipSprites_));
            if (version >= 0) archive(CEREAL_NVP(hoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardNoticeRow, 0)
