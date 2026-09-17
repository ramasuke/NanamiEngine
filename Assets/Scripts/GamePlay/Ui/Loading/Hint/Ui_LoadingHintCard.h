#pragma once
#include <string>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"

namespace GamePlay::Ui
{
    /**
     * @brief ロード画面に出す操作ヒント1行。
     *        ロード中はプレイヤーアバターが居ないので、パッド/キーボードの判定を自前で行う
     */
    class LoadingHintCard final : public Component::ComponentBase
    {
    public:
        /** @brief 表示を作り直す。ロード画面を出すたびに呼ぶ */
        void Reset();

    private:
        void ApplyGlyph(GameCore::PlayerAvatar::PlayerAvatarInputDevice device) const;

        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) text_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) glyph_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyboardGlyphSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padGlyphSprite_;
        [[serialize(0)]] std::string hintText_ = "回避の直後は無敵時間が発生する";

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(text_));
            archive(CEREAL_NVP(glyph_));
            archive(CEREAL_NVP(keyboardGlyphSprite_));
            archive(CEREAL_NVP(padGlyphSprite_));
            archive(CEREAL_NVP(hintText_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(text_));
            if (version >= 0) archive(CEREAL_NVP(glyph_));
            if (version >= 0) archive(CEREAL_NVP(keyboardGlyphSprite_));
            if (version >= 0) archive(CEREAL_NVP(padGlyphSprite_));
            if (version >= 0) archive(CEREAL_NVP(hintText_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::LoadingHintCard, 0)
