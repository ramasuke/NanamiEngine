#pragma once
#include <memory>
#include <string>

#include "../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Color/Color32.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/Component/CircleGaugeRenderer/CircleGaugeRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // 魔法陣の1枠。置き場所と大きさは SpellPalette が毎フレーム決め、枠内の配置は子オブジェクトの Transform が決める
    class SpellSlot final : public Component::ComponentBase
    {
    public:
        struct Appearance
        {
            float scale         = 1.0f;
            int   bodyAlpha     = 0;
            int   iconAlpha     = 0;
            int   activeAlpha   = 0;
            int   manaLackAlpha = 0;
            float cooldownRate  = 0.0f;
            int   cooldownAlpha = 0;
            int   textAlpha     = 0;
            int   glyphAlpha    = 0;
        };

        void SetSpell(const std::weak_ptr<Asset::SpriteFile>& icon, const std::string& costText);
        /// 消費 MP の色は都度補間できないので、足りなくなった瞬間・足りた瞬間にだけ差し替える
        void SetManaLack(bool isLacking);
        void SetCooldownText(const std::string& text);
        /** @brief ボタン表示の画像と、枠の中心から見た向き。外側の頂点に置く */
        void SetGlyph(const std::weak_ptr<Asset::SpriteFile>& glyph, const glm::vec2& direction);
        void Apply(const Appearance& appearance);

    private:
        void CatchParts();

        [[serialize(0)]] FIELD(GameObject::IGameObject) content_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) socket_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) active_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) icon_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) manaLackShade_;
        [[serialize(0)]] FIELD(NanamiUi::CircleGaugeRenderer) cooldown_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) cooldownText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) costPill_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) costText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) glyph_;
        [[serialize(0)]] Color32 costColor_ = Color32(214, 192, 255);
        [[serialize(0)]] Color32 costLackColor_ = Color32(255, 120, 104);
        /// 枠の中心からボタン表示までの距離（菱形の頂点）
        [[serialize(0)]] float glyphDistance_ = 33.3f;

        bool isPartsCaught_ = false;
        glm::vec3 contentBaseScale_ = glm::vec3(1.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(content_));
            archive(CEREAL_NVP(socket_));
            archive(CEREAL_NVP(active_));
            archive(CEREAL_NVP(icon_));
            archive(CEREAL_NVP(manaLackShade_));
            archive(CEREAL_NVP(cooldown_));
            archive(CEREAL_NVP(cooldownText_));
            archive(CEREAL_NVP(costPill_));
            archive(CEREAL_NVP(costText_));
            archive(CEREAL_NVP(glyph_));
            archive(CEREAL_NVP(costColor_));
            archive(CEREAL_NVP(costLackColor_));
            archive(CEREAL_NVP(glyphDistance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(content_));
            if (version >= 0) archive(CEREAL_NVP(socket_));
            if (version >= 0) archive(CEREAL_NVP(active_));
            if (version >= 0) archive(CEREAL_NVP(icon_));
            if (version >= 0) archive(CEREAL_NVP(manaLackShade_));
            if (version >= 0) archive(CEREAL_NVP(cooldown_));
            if (version >= 0) archive(CEREAL_NVP(cooldownText_));
            if (version >= 0) archive(CEREAL_NVP(costPill_));
            if (version >= 0) archive(CEREAL_NVP(costText_));
            if (version >= 0) archive(CEREAL_NVP(glyph_));
            if (version >= 0) archive(CEREAL_NVP(costColor_));
            if (version >= 0) archive(CEREAL_NVP(costLackColor_));
            if (version >= 0) archive(CEREAL_NVP(glyphDistance_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SpellSlot, 0)
