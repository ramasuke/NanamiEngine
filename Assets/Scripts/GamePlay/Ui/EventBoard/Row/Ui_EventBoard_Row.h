#pragma once
#include <functional>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../Model/EventBoardModel.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板に貼った告知1枚。行は表示窓の分だけ作って使い回すので、中身は Bind のたびに差し替える。
     */
    class EventBoardRow final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable
    {
    public:
        void Bind(const EventBoardEntry& entry);
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        void SetHighlighted(bool isHighlighted);

    private:
        void OnAwake() override;
        /** @brief 生成直後に Bind が来ても困らないよう、自前の参照はここで揃える */
        void EnsureComponents();
        void RefreshAppearance() const;

        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) noticeRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) titleText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) statusText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) ongoingStamp_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) waxSeal_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) selectedNoticeSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) unselectedNoticeSprite_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hoverSound_;
        [[serialize(0)]] float selectedScale_ = 1.05f;
        [[serialize(0)]] Color32 ongoingStatusColor_  = Color32(146, 38, 30);
        [[serialize(0)]] Color32 upcomingStatusColor_ = Color32(104, 78, 54);

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
            archive(CEREAL_NVP(statusText_));
            archive(CEREAL_NVP(ongoingStamp_));
            archive(CEREAL_NVP(waxSeal_));
            archive(CEREAL_NVP(selectedNoticeSprite_));
            archive(CEREAL_NVP(unselectedNoticeSprite_));
            archive(CEREAL_NVP(hoverSound_));
            archive(CEREAL_NVP(selectedScale_));
            archive(CEREAL_NVP(ongoingStatusColor_));
            archive(CEREAL_NVP(upcomingStatusColor_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(statusText_));
            if (version >= 0) archive(CEREAL_NVP(ongoingStamp_));
            if (version >= 0) archive(CEREAL_NVP(waxSeal_));
            if (version >= 0) archive(CEREAL_NVP(selectedNoticeSprite_));
            if (version >= 0) archive(CEREAL_NVP(unselectedNoticeSprite_));
            if (version >= 0) archive(CEREAL_NVP(hoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
            if (version >= 0) archive(CEREAL_NVP(ongoingStatusColor_));
            if (version >= 0) archive(CEREAL_NVP(upcomingStatusColor_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardRow, 0)
