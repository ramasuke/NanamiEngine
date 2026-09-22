#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"

namespace GamePlay::Ui
{
    // Each child ImageRenderer is one pip; the first `difficulty` children show the filled sprite.
    class StageDifficultyPips final : public Component::ComponentBase
    {
    public:
        void SetDifficulty(int difficulty);

    private:
        [[serialize(0)]] FIELD(Asset::SpriteFile) filledSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) emptySprite_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(filledSprite_));
            archive(CEREAL_NVP(emptySprite_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(filledSprite_));
            if (version >= 0) archive(CEREAL_NVP(emptySprite_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageDifficultyPips, 0)
