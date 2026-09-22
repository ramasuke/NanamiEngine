#pragma once
#include <memory>
#include <string>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/string.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Magic/IMagicSpell.h"
#include "../../Scripts/Core/Game/Magic/IMagicSpellEffect.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto MAGIC_SPELL_DATA_EXTENSION_LABEL = ".magicSpell";

    // 魔法1つ分の定義。共通の数値だけを持ち、何が起きるかは effect_ の実装に任せる
    class MagicSpellData final : public ScriptableObject,
                                 public GameCore::Magic::IMagicSpell
    {
    public:
        explicit MagicSpellData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&          DisplayName           () const override { return displayName_;            }
        [[nodiscard]] std::shared_ptr<SpriteFile> IconSprite            () const override { return iconSprite_.get();       }
        [[nodiscard]] std::shared_ptr<SoundFile>  CastSound             () const override { return castSound_.get();        }
        [[nodiscard]] float                       ManaCost              () const override { return manaCost_;               }
        [[nodiscard]] float                       Cooldown_secs         () const override { return cooldown_secs_;          }
        [[nodiscard]] float                       CastFireTime_secs     () const override { return castFireTime_secs_;      }
        [[nodiscard]] float                       CastTotalDuration_secs() const override { return castTotalDuration_secs_; }
        [[nodiscard]] GameCore::Magic::MagicCastMotion CastMotion       () const override { return castMotion_;             }
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> CastEffectPrefab() const override { return castEffectPrefab_.get(); }
        [[nodiscard]] const Guid&                 SpellGuid             () const override { return GetGuid();               }
        [[nodiscard]] GameCore::Magic::MagicCastTarget Aim(const GameCore::Magic::IMagicCaster& caster) const override;
        void Execute(const GameCore::Magic::IMagicCaster& caster, const GameCore::Magic::MagicCastTarget& target) const override;

    private:
        [[serialize(0)]] std::string       displayName_;
        [[serialize(0)]] FIELD(SpriteFile) iconSprite_;
        [[serialize(0)]] FIELD(SoundFile)  castSound_;
        [[serialize(0)]] float             manaCost_               = 10.0f;
        [[serialize(0)]] float             cooldown_secs_          = 1.0f;
        [[serialize(0)]] float             castFireTime_secs_      = 0.35f;
        [[serialize(0)]] float             castTotalDuration_secs_ = 0.75f;
        [[serialize(0)]] std::shared_ptr<GameCore::Magic::IMagicSpellEffect> effect_;
        [[serialize(1)]] GameCore::Magic::MagicCastMotion castMotion_ = GameCore::Magic::MagicCastMotion::OneHandThrust;
        [[serialize(1)]] FIELD(PrefabGameObjectFile) castEffectPrefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(iconSprite_));
            archive(CEREAL_NVP(castSound_));
            archive(CEREAL_NVP(manaCost_));
            archive(CEREAL_NVP(cooldown_secs_));
            archive(CEREAL_NVP(castFireTime_secs_));
            archive(CEREAL_NVP(castTotalDuration_secs_));
            archive(CEREAL_NVP(effect_));
            archive(CEREAL_NVP(castMotion_));
            archive(CEREAL_NVP(castEffectPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(iconSprite_));
            if (version >= 0) archive(CEREAL_NVP(castSound_));
            if (version >= 0) archive(CEREAL_NVP(manaCost_));
            if (version >= 0) archive(CEREAL_NVP(cooldown_secs_));
            if (version >= 0) archive(CEREAL_NVP(castFireTime_secs_));
            if (version >= 0) archive(CEREAL_NVP(castTotalDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(effect_));
            if (version >= 1) archive(CEREAL_NVP(castMotion_));
            if (version >= 1) archive(CEREAL_NVP(castEffectPrefab_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(MagicSpellData, MAGIC_SPELL_DATA_EXTENSION_LABEL, "Player::MagicCaster")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::MagicSpellData, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::MagicSpellData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::MagicSpellData);
#pragma endregion
