#pragma once
#include <string>

#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto MAGIC_CASTER_RESOURCE_EXTENSION_LABEL = ".magicCasterResource";

    class MagicCasterAvatarResource final : public ScriptableObject
    {
    public:
        explicit MagicCasterAvatarResource(const std::string& contentPath = "");
        /** 魔法弾が発射時に使うプレハブ */
        [[nodiscard]] PrefabGameObjectFile& MagicBoltPrefab() const { return *magicBoltPrefab_.get(); }
        [[nodiscard]] bool HasMagicBoltPrefab() const { return static_cast<bool>(magicBoltPrefab_); }
        [[nodiscard]] float MagicBoltSpeed() const { return magicBoltSpeed_; }
        /** Cast State開始からこの秒数経過した時点で弾を発射する */
        [[nodiscard]] float CastFireTime_secs() const { return castFireTime_secs_; }
        /** Cast State全体の長さ。経過したらIdleへ戻る */
        [[nodiscard]] float CastTotalDuration_secs() const { return castTotalDuration_secs_; }
        [[nodiscard]] float GroundCheckRadius  () const { return groundCheckRadius_;   }
        [[nodiscard]] float GroundCheckUpOffset() const { return groundCheckUpOffset_; }
        [[nodiscard]] float GroundCheckDistance() const { return groundCheckDistance_; }

    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) magicBoltPrefab_;
        [[serialize(0)]] float magicBoltSpeed_ = 220.0f;
        [[serialize(0)]] float castFireTime_secs_ = 0.35f;
        [[serialize(0)]] float castTotalDuration_secs_ = 0.75f;
        [[serialize(0)]] float groundCheckRadius_   = 40.0f;
        [[serialize(0)]] float groundCheckUpOffset_ = 3.0f;
        [[serialize(0)]] float groundCheckDistance_ = 8.3f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(magicBoltPrefab_));
            archive(CEREAL_NVP(magicBoltSpeed_));
            archive(CEREAL_NVP(castFireTime_secs_));
            archive(CEREAL_NVP(castTotalDuration_secs_));
            archive(CEREAL_NVP(groundCheckRadius_));
            archive(CEREAL_NVP(groundCheckUpOffset_));
            archive(CEREAL_NVP(groundCheckDistance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(magicBoltPrefab_));
            archive(CEREAL_NVP(magicBoltSpeed_));
            archive(CEREAL_NVP(castFireTime_secs_));
            archive(CEREAL_NVP(castTotalDuration_secs_));
            archive(CEREAL_NVP(groundCheckRadius_));
            archive(CEREAL_NVP(groundCheckUpOffset_));
            archive(CEREAL_NVP(groundCheckDistance_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(MagicCasterAvatarResource, MAGIC_CASTER_RESOURCE_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::MagicCasterAvatarResource, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::MagicCasterAvatarResource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::MagicCasterAvatarResource);
#pragma endregion
