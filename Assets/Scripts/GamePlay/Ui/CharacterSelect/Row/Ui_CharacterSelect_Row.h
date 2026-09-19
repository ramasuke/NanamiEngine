#pragma once
#include <functional>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Data/Character/Data_CharacterData.h"

namespace GamePlay::Ui
{
    /**
     * @brief 酒場の板に打ち付けた手配書1枚。名簿の1行にあたる。
     * 中身は実行時に Initialize で流し込むので、行ごとのアセット参照は持たない。
     */
    class CharacterSelectRow final : public Component::ComponentBase,
                                     public LifeCycleCallback::IAwakable
    {
    public:
        void Initialize(const std::shared_ptr<Asset::CharacterData>& character);
        void SubscribeOnClickSelectButton(std::function<void()> onClick);
        void SetHighlighted(bool isHighlighted);

        [[nodiscard]] std::shared_ptr<Asset::CharacterData> Data() const { return character_; }

    private:
        void OnAwake() override;
        /** @brief 生成直後に Initialize が来ても困らないよう、自前の参照はここで揃える */
        void EnsureComponents();
        void RefreshAppearance() const;

        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) billRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) readingText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) taglineText_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) waxSeal_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) lockedStamp_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) selectedBillSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) unselectedBillSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) lockedBillSprite_;
        [[serialize(0)]] FIELD(Asset::SoundFile) hoverSound_;
        // 選択中の手配書は一回り大きく見せる
        [[serialize(0)]] float selectedScale_ = 1.08f;

        std::shared_ptr<Asset::CharacterData> character_;
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
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(readingText_));
            archive(CEREAL_NVP(taglineText_));
            archive(CEREAL_NVP(waxSeal_));
            archive(CEREAL_NVP(lockedStamp_));
            archive(CEREAL_NVP(selectedBillSprite_));
            archive(CEREAL_NVP(unselectedBillSprite_));
            archive(CEREAL_NVP(lockedBillSprite_));
            archive(CEREAL_NVP(hoverSound_));
            archive(CEREAL_NVP(selectedScale_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(readingText_));
            if (version >= 0) archive(CEREAL_NVP(taglineText_));
            if (version >= 0) archive(CEREAL_NVP(waxSeal_));
            if (version >= 0) archive(CEREAL_NVP(lockedStamp_));
            if (version >= 0) archive(CEREAL_NVP(selectedBillSprite_));
            if (version >= 0) archive(CEREAL_NVP(unselectedBillSprite_));
            if (version >= 0) archive(CEREAL_NVP(lockedBillSprite_));
            if (version >= 0) archive(CEREAL_NVP(hoverSound_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::CharacterSelectRow, 0)
