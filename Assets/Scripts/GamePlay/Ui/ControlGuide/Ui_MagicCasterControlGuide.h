#pragma once
#include <array>
#include <memory>
#include <string>
#include "Ui_ControlGuide.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/MagicCaster/State/Transition/MagicCasterAvatarStateTransition.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar;
}

namespace GamePlay::Ui
{
    // State が宣言する遷移と操作から、どの行に何を出すかを決める。見せ方は controlGuide_ が行う。
    // 魔法の枠（LT+ABXY / 1〜4）はスペルパレットが出すので、ここでは基本魔法だけを扱う
    class MagicCasterControlGuide final : public Component::ComponentBase,
                                          public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& magicCasterAvatar);

    private:
        /// State が宣言する遷移と操作を、ガイドの行へ振り分ける
        class RequestCollector;

        // 押す操作そのもの。実際に出す絵は接続中の入力機器で選ぶ
        enum class Glyph : std::uint8_t
        {
            Move,
            Cast,
            Run,
            Jump,
            LockOn,
            Interact,
        };

        enum class Label : std::uint8_t
        {
            Move,
            Cast,
            Run,
            Jump,
            LockOn,
            LockOnRelease,
            Chat,
        };

        // 下から並ぶ順。行はこの順に生成する
        enum class Row : std::uint8_t
        {
            Move,
            Cast,
            Run,
            Jump,
            LockOn,
            Interact,
            Count,
        };

        struct RowRequest
        {
            bool  isShown  = false;
            bool  isUsable = false;
            Glyph glyph    = Glyph::Move;
            Label label    = Label::Move;
        };

        using RowRequests = std::array<RowRequest, static_cast<std::size_t>(Row::Count)>;

        void OnUpdate() override;

        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> GlyphSprite(Glyph glyph) const;
        [[nodiscard]] const std::string& LabelText(Label label) const;

        [[serialize(0)]] FIELD(Ui::ControlGuide) controlGuide_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) keyMoveSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyCastSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyRunSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyJumpSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyLockOnSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyInteractSprite_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) padMoveSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padCastSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padRunSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padJumpSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padLockOnSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padInteractSprite_;

        [[serialize(0)]] std::string moveLabel_;
        [[serialize(0)]] std::string castLabel_;
        [[serialize(0)]] std::string runLabel_;
        [[serialize(0)]] std::string jumpLabel_;
        [[serialize(0)]] std::string lockOnLabel_;
        [[serialize(0)]] std::string lockOnReleaseLabel_;
        [[serialize(0)]] std::string chatLabel_;

        std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar> magicCasterAvatar_;
        RowRequests requests_{};
        GameCore::PlayerAvatar::PlayerAvatarInputDevice device_ = GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(controlGuide_));
            archive(CEREAL_NVP(keyMoveSprite_));
            archive(CEREAL_NVP(keyCastSprite_));
            archive(CEREAL_NVP(keyRunSprite_));
            archive(CEREAL_NVP(keyJumpSprite_));
            archive(CEREAL_NVP(keyLockOnSprite_));
            archive(CEREAL_NVP(keyInteractSprite_));
            archive(CEREAL_NVP(padMoveSprite_));
            archive(CEREAL_NVP(padCastSprite_));
            archive(CEREAL_NVP(padRunSprite_));
            archive(CEREAL_NVP(padJumpSprite_));
            archive(CEREAL_NVP(padLockOnSprite_));
            archive(CEREAL_NVP(padInteractSprite_));
            archive(CEREAL_NVP(moveLabel_));
            archive(CEREAL_NVP(castLabel_));
            archive(CEREAL_NVP(runLabel_));
            archive(CEREAL_NVP(jumpLabel_));
            archive(CEREAL_NVP(lockOnLabel_));
            archive(CEREAL_NVP(lockOnReleaseLabel_));
            archive(CEREAL_NVP(chatLabel_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(controlGuide_));
            if (version >= 0) archive(CEREAL_NVP(keyMoveSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyCastSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyRunSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyJumpSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyLockOnSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyInteractSprite_));
            if (version >= 0) archive(CEREAL_NVP(padMoveSprite_));
            if (version >= 0) archive(CEREAL_NVP(padCastSprite_));
            if (version >= 0) archive(CEREAL_NVP(padRunSprite_));
            if (version >= 0) archive(CEREAL_NVP(padJumpSprite_));
            if (version >= 0) archive(CEREAL_NVP(padLockOnSprite_));
            if (version >= 0) archive(CEREAL_NVP(padInteractSprite_));
            if (version >= 0) archive(CEREAL_NVP(moveLabel_));
            if (version >= 0) archive(CEREAL_NVP(castLabel_));
            if (version >= 0) archive(CEREAL_NVP(runLabel_));
            if (version >= 0) archive(CEREAL_NVP(jumpLabel_));
            if (version >= 0) archive(CEREAL_NVP(lockOnLabel_));
            if (version >= 0) archive(CEREAL_NVP(lockOnReleaseLabel_));
            if (version >= 0) archive(CEREAL_NVP(chatLabel_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::MagicCasterControlGuide, 0)
