#include "Data_SwordManAvatarResource.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    SwordManAvatarResource::SwordManAvatarResource(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void SwordManAvatarResource::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("normalAttackParticlePrefab_",normalAttackParticlePrefab_);
        ImGuiHelper::OnDrawInputField("dealDamageTextBillBoardPrefab_",dealDamageTextBillBoardPrefab_);
        ImGuiHelper::OnDrawInputField("attackWhiffSound_", attackWhiffSound_);
        ImGuiHelper::OnDrawInputField("attackHitSound_", attackHitSound_);
        ImGuiHelper::OnDrawInputField("dashAttackWhiffSound_", dashAttackWhiffSound_);
        ImGuiHelper::OnDrawInputField("dashAttackHitSound_", dashAttackHitSound_);
        ImGuiHelper::OnDrawInputField("jumpAttackPlungeSound_", jumpAttackPlungeSound_);
        ImGuiHelper::OnDrawInputField("jumpAttackWhiffSound_", jumpAttackWhiffSound_);
        ImGuiHelper::OnDrawInputField("jumpAttackHitSound_", jumpAttackHitSound_);
        ImGuiHelper::OnDrawInputField("avoidRollingSound_", avoidRollingSound_);
        ImGuiHelper::OnDrawInputField("justAvoidRollingSound_", justAvoidRollingSound_);
        ImGuiHelper::OnDrawInputField("jumpSound_", jumpSound_);
        ImGuiHelper::OnDrawInputField("footstepParticlePrefab_", footstepParticlePrefab_);
        ImGuiHelper::OnDrawInputField("landingParticlePrefab_", landingParticlePrefab_);
        ImGuiHelper::OnDrawInputField("landingParticleMinFallSpeed_", landingParticleMinFallSpeed_);
        ImGuiHelper::OnDrawInputField("landingParticleMaxFallSpeed_", landingParticleMaxFallSpeed_);
        ImGuiHelper::OnDrawInputField("landingParticleMinScale_", landingParticleMinScale_);
        ImGuiHelper::OnDrawInputField("landingParticleMaxScale_", landingParticleMaxScale_);
        ImGuiHelper::OnDrawInputField("chargeCompleteSound_", chargeCompleteSound_);
        ImGuiHelper::OnDrawInputField("chargeCompleteParticlePrefab_", chargeCompleteParticlePrefab_);
        ImGuiHelper::OnDrawInputField("chargeHoldParticlePrefab_", chargeHoldParticlePrefab_);
        ImGuiHelper::OnDrawInputField("chargeImpactParticlePrefab_", chargeImpactParticlePrefab_);
        ImGuiHelper::OnDrawInputField("attackBlockedParticlePrefab_", attackBlockedParticlePrefab_);
        ImGuiHelper::OnDrawInputField("chargingShakeIntensityMin_", chargingShakeIntensityMin_);
        ImGuiHelper::OnDrawInputField("chargingShakeIntensityMax_", chargingShakeIntensityMax_);
        ImGuiHelper::OnDrawInputField("chargedHoldShakeIntensity_", chargedHoldShakeIntensity_);
        ImGuiHelper::OnDrawInputField("chargeCompleteShakeIntensity_", chargeCompleteShakeIntensity_);
        ImGuiHelper::OnDrawInputField("chargeCompleteShakeDuration_secs_", chargeCompleteShakeDuration_secs_);
        ImGuiHelper::OnDrawInputField("groundCheckRadius_", groundCheckRadius_);
        ImGuiHelper::OnDrawInputField("groundCheckUpOffset_", groundCheckUpOffset_);
        ImGuiHelper::OnDrawInputField("groundCheckDistance_", groundCheckDistance_);
        ImGuiHelper::OnDrawInputField("maxWalkableSlope_deg_", maxWalkableSlope_deg_);
        ImGuiHelper::OnDrawInputField("slopeCheckRadius_", slopeCheckRadius_);
        ImGuiHelper::OnDrawInputField("slopeCheckUpOffset_", slopeCheckUpOffset_);
        ImGuiHelper::OnDrawInputField("slopeCheckDistance_", slopeCheckDistance_);
        ImGuiHelper::OnDrawInputField("walkAccelerationTime_secs_", walkAccelerationTime_secs_);
        ImGuiHelper::OnDrawInputField("runAccelerationTime_secs_", runAccelerationTime_secs_);
        ImGuiHelper::OnDrawInputField("walkDecelerationTime_secs_", walkDecelerationTime_secs_);
        ImGuiHelper::OnDrawInputField("runDecelerationTime_secs_", runDecelerationTime_secs_);
        ImGuiHelper::OnDrawInputField("footstepContactHeight_", footstepContactHeight_);
        ImGuiHelper::OnDrawInputField("footstepBoneNames_", footstepBoneNames_, [this]
        {
            if (ImGui::Button("Add Footstep Bone"))
                footstepBoneNames_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("walkFootstepSounds_", walkFootstepSounds_, [this]
        {
            if (ImGui::Button("Add Walk Footstep Sound"))
                walkFootstepSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("runFootstepSounds_", runFootstepSounds_, [this]
        {
            if (ImGui::Button("Add Run Footstep Sound"))
                runFootstepSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("attackBlockedSounds_", attackBlockedSounds_, [this]
        {
            if (ImGui::Button("Add Attack Blocked Sound"))
                attackBlockedSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("comboNormalAttackWhiffSounds_", comboNormalAttackWhiffSounds_, [this]
        {
            if (ImGui::Button("Add Combo Normal Attack Whiff Sound"))
                comboNormalAttackWhiffSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("comboNormalAttackHitSounds_", comboNormalAttackHitSounds_, [this]
        {
            if (ImGui::Button("Add Combo Normal Attack Hit Sound"))
                comboNormalAttackHitSounds_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("initialItems_", initialItems_, [this]
        {
            if (ImGui::Button("Add Initial Item"))
                initialItems_.emplace_back();
        });
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(SwordManAvatarResource, SWORD_MAN_RESOURCE_EXTENSION_LABEL, "Player::SwordMan")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SwordManAvatarResource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::SwordManAvatarResource);
#pragma endregion
