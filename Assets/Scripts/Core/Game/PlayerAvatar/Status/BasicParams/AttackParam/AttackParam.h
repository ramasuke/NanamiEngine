#pragma once
#include "../../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../EnahancePower/EnhancePower.h"

namespace GameCore::PlayerAvatar
{
    template<typename T>
    struct AttackParam final
    {
        explicit AttackParam(
            T attackPower = T(),
            EnhancePower attackedEnhanceValue = EnhancePower(),
            float normalAttackDuration_secs = 0.0f,
            float normalAttackOccurrenceDuration_secs = 0.0f);
        [[nodiscard]] const T&            AttackPower            () const { return attackPower_; }
        [[nodiscard]] const EnhancePower& GetEnhance             () const { return enhance_    ; }
        [[nodiscard]] float               Duration_secs          () const { return normalAttackDuration_secs_;           }
        [[nodiscard]] float               OccurrenceDuration_secs() const { return normalAttackOccurrenceDuration_secs_; }
        
    private:
        [[serialize(0)]] T attackPower_; 
        [[serialize(0)]] EnhancePower enhance_;
        [[serialize(0)]] float normalAttackOccurrenceDuration_secs_;
        [[serialize(0)]] float normalAttackDuration_secs_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(attackPower_);
            archive(enhance_);
            archive(normalAttackOccurrenceDuration_secs_);
            archive(normalAttackDuration_secs_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(attackPower_);
            archive(enhance_);
            archive(normalAttackOccurrenceDuration_secs_);
            archive(normalAttackDuration_secs_);
        }
#pragma endregion
    };

    template <typename T>
    AttackParam<T>::AttackParam(
        T attackPower,
        const EnhancePower attackedEnhanceValue,
        const float normalAttackOccurrenceDuration_secs,
        const float normalAttackDuration_secs)
            : attackPower_(attackPower)
            , enhance_(attackedEnhanceValue)
            , normalAttackOccurrenceDuration_secs_(normalAttackOccurrenceDuration_secs)
            , normalAttackDuration_secs_(normalAttackDuration_secs)
    {
        
    }

    template <typename T>
    void AttackParam<T>::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("attackPower_", attackPower_);
        LibCore::ImGuiHelper::OnDrawInputField("enhance_", enhance_);
        LibCore::ImGuiHelper::OnDrawInputField("normalAttackOccurrenceDuration_secs_", normalAttackOccurrenceDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("normalAttackDuration_secs_", normalAttackDuration_secs_);
    }
}
