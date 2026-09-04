#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"

#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto SWORD_MAN_RESOURCE_EXTENSION_LABEL = ".swordManResource";

    class SwordManAvatarResource final : public ScriptableObject
    {
    public:
        explicit SwordManAvatarResource(const std::string& contentPath = "");
        [[nodiscard]] PrefabGameObjectFile& NormalAttackParticlePrefab() const { return *normalAttackParticlePrefab_.get(); }
        [[nodiscard]] PrefabGameObjectFile& DealDamageTextBillBoardPrefab() const { return *dealDamageTextBillBoardPrefab_.get(); }
        [[nodiscard]] SoundFile& NormalAttackSound    () const { return *normalAttackSound_    .get(); }
        [[nodiscard]] SoundFile& AvoidRollingSound    () const { return *avoidRollingSound_    .get(); }
        [[nodiscard]] SoundFile& JustAvoidRollingSound() const { return *justAvoidRollingSound_.get(); }
        [[nodiscard]] SoundFile& JumpSound() const { return *jumpSound_.get(); }

        [[nodiscard]] PrefabGameObjectFile& FootstepParticlePrefab() const { return *footstepParticlePrefab_.get(); }
        [[nodiscard]] bool HasFootstepParticlePrefab() const { return static_cast<bool>(footstepParticlePrefab_); }
        /** 歩き系クリップ（Walk / InjuredWalk）の接地タイミング。クリップ正規化時間 [0,1) の配列 */
        [[nodiscard]] const std::vector<float>& WalkFootstepContactPhases() const { return walkFootstepContactPhases_; }
        /** 走り系クリップ（Run / InjuredRun）の接地タイミング。クリップ正規化時間 [0,1) の配列 */
        [[nodiscard]] const std::vector<float>& RunFootstepContactPhases()  const { return runFootstepContactPhases_; }
        /** 歩き系クリップの足音候補。鳴らすときに配列からランダムで1つ選択する */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& WalkFootstepSounds() const { return walkFootstepSounds_; }
        /** 走り系クリップの足音候補。鳴らすときに配列からランダムで1つ選択する */
        [[nodiscard]] const std::vector<FIELD(SoundFile)>& RunFootstepSounds()  const { return runFootstepSounds_; }

    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) normalAttackParticlePrefab_;
        [[serialize(2)]] FIELD(PrefabGameObjectFile) dealDamageTextBillBoardPrefab_;
        [[serialize(0)]] FIELD(SoundFile) normalAttackSound_;
        [[serialize(0)]] FIELD(SoundFile) avoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) justAvoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) jumpSound_;
        [[serialize(3)]] FIELD(PrefabGameObjectFile) footstepParticlePrefab_;
        [[serialize(3)]] std::vector<float>          walkFootstepContactPhases_;
        [[serialize(3)]] std::vector<float>          runFootstepContactPhases_;
        [[serialize(4)]] std::vector<FIELD(SoundFile)> walkFootstepSounds_;
        [[serialize(4)]] std::vector<FIELD(SoundFile)> runFootstepSounds_;
        
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(normalAttackParticlePrefab_));
            archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
            archive(CEREAL_NVP(normalAttackSound_));
            archive(CEREAL_NVP(avoidRollingSound_));
            archive(CEREAL_NVP(justAvoidRollingSound_));
            archive(CEREAL_NVP(jumpSound_));
            archive(CEREAL_NVP(footstepParticlePrefab_));
            archive(CEREAL_NVP(walkFootstepContactPhases_));
            archive(CEREAL_NVP(runFootstepContactPhases_));

            archive(cereal::make_nvp("walkFootstepSoundCount", static_cast<std::uint32_t>(walkFootstepSounds_.size())));
            for (size_t i = 0; i < walkFootstepSounds_.size(); ++i)
                archive(cereal::make_nvp("walkFootstepSound_" + std::to_string(i), walkFootstepSounds_[i]));

            archive(cereal::make_nvp("runFootstepSoundCount", static_cast<std::uint32_t>(runFootstepSounds_.size())));
            for (size_t i = 0; i < runFootstepSounds_.size(); ++i)
                archive(cereal::make_nvp("runFootstepSound_" + std::to_string(i), runFootstepSounds_[i]));
        }
        
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(normalAttackParticlePrefab_));
            if (version >= 2) archive(CEREAL_NVP(dealDamageTextBillBoardPrefab_));
            if (version >= 0) archive(CEREAL_NVP(normalAttackSound_));
            if (version >= 0) archive(CEREAL_NVP(avoidRollingSound_));
            if (version >= 0) archive(CEREAL_NVP(justAvoidRollingSound_));
            if (version >= 1) archive(CEREAL_NVP(jumpSound_));
            if (version >= 3) archive(CEREAL_NVP(footstepParticlePrefab_));
            if (version >= 3) archive(CEREAL_NVP(walkFootstepContactPhases_));
            if (version >= 3) archive(CEREAL_NVP(runFootstepContactPhases_));
            if (version >= 4)
            {
                std::uint32_t walkFootstepSoundCount = 0;
                archive(cereal::make_nvp("walkFootstepSoundCount", walkFootstepSoundCount));
                walkFootstepSounds_.resize(walkFootstepSoundCount);
                for (size_t i = 0; i < walkFootstepSoundCount; ++i)
                    archive(cereal::make_nvp("walkFootstepSound_" + std::to_string(i), walkFootstepSounds_[i]));

                std::uint32_t runFootstepSoundCount = 0;
                archive(cereal::make_nvp("runFootstepSoundCount", runFootstepSoundCount));
                runFootstepSounds_.resize(runFootstepSoundCount);
                for (size_t i = 0; i < runFootstepSoundCount; ++i)
                    archive(cereal::make_nvp("runFootstepSound_" + std::to_string(i), runFootstepSounds_[i]));
            }
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(SwordManAvatarResource, SWORD_MAN_RESOURCE_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SwordManAvatarResource, 4);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SwordManAvatarResource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::SwordManAvatarResource);
#pragma endregion
