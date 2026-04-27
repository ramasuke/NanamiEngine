#pragma once
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto FRIENDLY_NPC_BASE_RESOURCES_DATA_EXTENSION_LABEL = ".friendlyNpcResources";

    /** Npcが使用する基本Status */
    class FriendlyNpcResources final : public ScriptableObject
    {
    public:
        explicit FriendlyNpcResources(const std::string& contentPath = "");
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> GetChattingIconPrefab () const { return chattingIconPrefab_ .get(); }
        [[nodiscard]] std::shared_ptr<PrefabGameObjectFile> GetChattableIconPrefab() const { return chattableIconPrefab_.get(); }
        
    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) chattingIconPrefab_ ;
        [[serialize(1)]] FIELD(PrefabGameObjectFile) chattableIconPrefab_;
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(chattingIconPrefab_));
            archive(CEREAL_NVP(chattableIconPrefab_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(chattingIconPrefab_));
            if (version >= 1) archive(CEREAL_NVP(chattableIconPrefab_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(FriendlyNpcResources, FRIENDLY_NPC_BASE_RESOURCES_DATA_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::FriendlyNpcResources, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::FriendlyNpcResources);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::FriendlyNpcResources);
#pragma endregion
