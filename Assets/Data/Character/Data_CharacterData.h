#pragma once
#include <string>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto CHARACTER_DATA_EXTENSION_LABEL = ".characterData";

    /**
     * @brief 酒場のキャラ選択に出す一人ぶんの定義。
     * 見た目のプレビューは展示台に立てる displayModelPrefab_ が担う(UIに3Dを描く仕組みは無い)。
     */
    class CharacterData final : public ScriptableObject
    {
    public:
        explicit CharacterData(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              DisplayName     () const { return displayName_;      }
        [[nodiscard]] const std::string&              Reading         () const { return reading_;          }
        [[nodiscard]] const std::string&              Tagline         () const { return tagline_;          }
        [[nodiscard]] GameCore::PlayerAvatar::PlayerAvatarType AvatarType() const { return avatarType_;    }
        [[nodiscard]] bool                            IsUnlocked      () const { return isUnlocked_;       }
        [[nodiscard]] int                             PowerPips       () const { return powerPips_;        }
        [[nodiscard]] int                             ToughnessPips   () const { return toughnessPips_;    }
        [[nodiscard]] int                             AgilityPips     () const { return agilityPips_;      }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> DisplayModelPrefab() const { return displayModelPrefab_.get(); }

    private:
        [[serialize(0)]] std::string                                  displayName_;
        [[serialize(0)]] std::string                                  reading_;
        [[serialize(0)]] std::string                                  tagline_;
        [[serialize(0)]] GameCore::PlayerAvatar::PlayerAvatarType     avatarType_ = GameCore::PlayerAvatar::PlayerAvatarType::SwordMan;
        [[serialize(0)]] bool                                         isUnlocked_ = true;
        [[serialize(0)]] int                                          powerPips_ = 3;
        [[serialize(0)]] int                                          toughnessPips_ = 3;
        [[serialize(0)]] int                                          agilityPips_ = 3;
        [[serialize(0)]] std::vector<std::string>                     descriptionLines_;
        [[serialize(0)]] FIELD(PrefabGameObjectFile)                  displayModelPrefab_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(displayName_));
            archive(CEREAL_NVP(reading_));
            archive(CEREAL_NVP(tagline_));
            archive(CEREAL_NVP(avatarType_));
            archive(CEREAL_NVP(isUnlocked_));
            archive(CEREAL_NVP(powerPips_));
            archive(CEREAL_NVP(toughnessPips_));
            archive(CEREAL_NVP(agilityPips_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(displayModelPrefab_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(displayName_));
            if (version >= 0) archive(CEREAL_NVP(reading_));
            if (version >= 0) archive(CEREAL_NVP(tagline_));
            if (version >= 0) archive(CEREAL_NVP(avatarType_));
            if (version >= 0) archive(CEREAL_NVP(isUnlocked_));
            if (version >= 0) archive(CEREAL_NVP(powerPips_));
            if (version >= 0) archive(CEREAL_NVP(toughnessPips_));
            if (version >= 0) archive(CEREAL_NVP(agilityPips_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(displayModelPrefab_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(CharacterData, CHARACTER_DATA_EXTENSION_LABEL, "Player")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::CharacterData, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::CharacterData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::CharacterData);
#pragma endregion
