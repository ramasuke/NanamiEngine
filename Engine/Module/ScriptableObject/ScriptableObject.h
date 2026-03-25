#pragma once
#include "../Asset/AssetBase.h"
#include "../Asset/Factory/AssetFactory.h"
#include "../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "NullContextFile/NullContextFile.h"

namespace NanamiEngine::Module
{
    class ScriptableObject : public Asset::AssetBase,
                             public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit ScriptableObject(std::string contentPath = "");
        virtual ~ScriptableObject() override = default;
        [[nodiscard]] const Guid& GetGuid()        const override { return guid_;           }
        [[nodiscard]] std::string GetContentPath() const override { return contentPath_; }

    private:
        void OnEnableAsset() override;
        void OnSaveCallback() override;
        

        std::string contentPath_;
        Guid guid_;
        
#pragma region Serialization Function
    public:
        virtual void OnDrawGui() {
            LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
            LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<AssetBase>(this));
            archive(CEREAL_NVP(contentPath_));
            archive(CEREAL_NVP(guid_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<AssetBase>(this));
            if (version >= 0) archive(CEREAL_NVP(contentPath_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::ScriptableObject, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::ScriptableObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::ScriptableObject);
#pragma endregion

#define REGISTER_SCRIPTABLE_OBJECT(TYPE, EXTENSION_LABEL) \
REGISTER_ASSET(TYPE, EXTENSION_LABEL)                     \
REGISTER_CREATABLE_ASSET_EXTENSION(#TYPE, EXTENSION_LABEL)
