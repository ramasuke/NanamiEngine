#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto FRIENDLY_NPC_BASE_RESOURCES_DATA_EXTENSION_LABEL = ".friendBehaviourData";

    /** Npcが使用する基本Status */
    class FriendlyNpcBaseResources final : public ScriptableObject
    {
    public:
        explicit FriendlyNpcBaseResources(const std::string& contentPath = "");
        [[nodiscard]] const SpriteFile& GetChattingIconSprite() const { return *chattingIconSprite_.get(); }
        
    private:
        [[serialize(0)]] FIELD(SpriteFile) chattingIconSprite_;
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(chattingIconSprite_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(chattingIconSprite_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(FriendlyNpcBaseResources, FRIENDLY_NPC_BASE_RESOURCES_DATA_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::FriendlyNpcBaseResources, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::FriendlyNpcBaseResources);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::FriendlyNpcBaseResources);
#pragma endregion
