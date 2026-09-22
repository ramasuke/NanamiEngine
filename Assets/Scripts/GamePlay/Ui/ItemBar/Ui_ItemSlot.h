#pragma once
#include <memory>
#include <string>

#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // アイテム欄の1枠。枠の並びは親の HorizontalLayoutGroup、枠内の配置は子オブジェクトの Transform が決める。
    // 何を出すかと明るさは ItemBar が毎フレーム渡す
    class ItemSlot final : public Component::ComponentBase
    {
    public:
        struct Appearance
        {
            float scale             = 1.0f;
            int   bodyAlpha         = 0;
            int   iconAlpha         = 0;
            int   frameAlpha        = 0;
            int   selectedFrameAlpha = 0;
            int   selectGlowAlpha   = 0;
            int   countAlpha        = 0;
        };

        void SetContent(const std::weak_ptr<Asset::SpriteFile>& icon, const std::string& countText);
        /// 個数の色は都度補間できないので、選ばれた瞬間・外れた瞬間にだけ差し替える
        void SetSelected(bool isSelected);
        void Apply(const Appearance& appearance);

    private:
        void CatchParts();

        [[serialize(0)]] FIELD(GameObject::IGameObject) content_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) backing_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) icon_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) frame_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) selectedFrame_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) selectGlow_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) countPill_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) countText_;
        [[serialize(0)]] Color32 countColor_ = Color32(196, 208, 212);
        [[serialize(0)]] Color32 countSelectedColor_ = Color32(255, 206, 104);

        bool isPartsCaught_ = false;
        glm::vec3 contentBaseScale_ = glm::vec3(1.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(content_));
            archive(CEREAL_NVP(backing_));
            archive(CEREAL_NVP(icon_));
            archive(CEREAL_NVP(frame_));
            archive(CEREAL_NVP(selectedFrame_));
            archive(CEREAL_NVP(selectGlow_));
            archive(CEREAL_NVP(countPill_));
            archive(CEREAL_NVP(countText_));
            archive(CEREAL_NVP(countColor_));
            archive(CEREAL_NVP(countSelectedColor_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(content_));
            if (version >= 0) archive(CEREAL_NVP(backing_));
            if (version >= 0) archive(CEREAL_NVP(icon_));
            if (version >= 0) archive(CEREAL_NVP(frame_));
            if (version >= 0) archive(CEREAL_NVP(selectedFrame_));
            if (version >= 0) archive(CEREAL_NVP(selectGlow_));
            if (version >= 0) archive(CEREAL_NVP(countPill_));
            if (version >= 0) archive(CEREAL_NVP(countText_));
            if (version >= 0) archive(CEREAL_NVP(countColor_));
            if (version >= 0) archive(CEREAL_NVP(countSelectedColor_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ItemSlot, 0)
