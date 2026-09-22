#pragma once
#include <functional>
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板の上に吊った木札の見出し1枚。選ばれた札は明るくして少し下へ垂らす。
     * 未読があれば右肩に封蝋の数字を出す。
     */
    class EventBoardTab final : public Component::ComponentBase,
                                public LifeCycleCallback::IAwakable
    {
    public:
        /** @brief 吊る位置。選ばれたときはここから selectedDrop_px_ だけ下げる */
        void Place(const glm::vec3& localPos);
        void SetLabel(const std::string& label);
        void SetSelected(bool isSelected);
        /** @param count 0 ならバッジを隠す */
        void SetBadgeCount(size_t count);
        void SubscribeOnClick(std::function<void()> onClick);

    private:
        void OnAwake() override;
        /** @brief 生成直後に Set* が来ても困らないよう、自前の参照はここで揃える */
        void EnsureComponents();

        FIELD(NanamiUi::Button) selectButton_;
        FIELD(Component::ImageRenderer) boardRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) labelText_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) badgeRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) badgeCountText_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) selectedSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) unselectedSprite_;
        [[serialize(0)]] Color32 selectedLabelColor_   = Color32(255, 234, 190);
        [[serialize(0)]] Color32 unselectedLabelColor_ = Color32(168, 146, 116);
        [[serialize(0)]] float selectedScale_   = 1.06f;
        [[serialize(0)]] float selectedDrop_px_ = 8.0f;

        glm::vec3 basePos_   = glm::vec3(0.0f);
        glm::vec3 baseScale_ = glm::vec3(1.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(labelText_));
            archive(CEREAL_NVP(badgeRoot_));
            archive(CEREAL_NVP(badgeCountText_));
            archive(CEREAL_NVP(selectedSprite_));
            archive(CEREAL_NVP(unselectedSprite_));
            archive(CEREAL_NVP(selectedLabelColor_));
            archive(CEREAL_NVP(unselectedLabelColor_));
            archive(CEREAL_NVP(selectedScale_));
            archive(CEREAL_NVP(selectedDrop_px_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(labelText_));
            if (version >= 0) archive(CEREAL_NVP(badgeRoot_));
            if (version >= 0) archive(CEREAL_NVP(badgeCountText_));
            if (version >= 0) archive(CEREAL_NVP(selectedSprite_));
            if (version >= 0) archive(CEREAL_NVP(unselectedSprite_));
            if (version >= 0) archive(CEREAL_NVP(selectedLabelColor_));
            if (version >= 0) archive(CEREAL_NVP(unselectedLabelColor_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
            if (version >= 0) archive(CEREAL_NVP(selectedDrop_px_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardTab, 0)
