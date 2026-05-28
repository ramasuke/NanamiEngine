#pragma once
#include <string>

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
        [[nodiscard]] SoundFile& NormalAttackSound    () const { return *normalAttackSound_    .get(); }
        [[nodiscard]] SoundFile& AvoidRollingSound    () const { return *avoidRollingSound_    .get(); }
        [[nodiscard]] SoundFile& JustAvoidRollingSound() const { return *justAvoidRollingSound_.get(); }
        [[nodiscard]] SoundFile& JumpSound() const { return *jumpSound_.get(); }
        
    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) normalAttackParticlePrefab_;
        [[serialize(0)]] FIELD(SoundFile) normalAttackSound_;
        [[serialize(0)]] FIELD(SoundFile) avoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) justAvoidRollingSound_;
        [[serialize(0)]] FIELD(SoundFile) jumpSound_;
        
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(normalAttackParticlePrefab_));
            archive(CEREAL_NVP(normalAttackSound_));
            archive(CEREAL_NVP(avoidRollingSound_));
            archive(CEREAL_NVP(justAvoidRollingSound_));
            archive(CEREAL_NVP(jumpSound_));
        }
        
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(normalAttackParticlePrefab_));
            if (version >= 0) archive(CEREAL_NVP(normalAttackSound_));
            if (version >= 0) archive(CEREAL_NVP(avoidRollingSound_));
            if (version >= 0) archive(CEREAL_NVP(justAvoidRollingSound_));
            if (version >= 1) archive(CEREAL_NVP(jumpSound_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(SwordManAvatarResource, SWORD_MAN_RESOURCE_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SwordManAvatarResource, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SwordManAvatarResource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::SwordManAvatarResource);
#pragma endregion
