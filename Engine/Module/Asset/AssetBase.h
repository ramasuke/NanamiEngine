#pragma once
#include "../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Asset
{
    class AssetBase : public Object::IObject
    {
    public:
        ~AssetBase() override = default;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            
        }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            
        }

        [[nodiscard]] virtual std::string GetContentPath() const = 0;
        virtual void OnDoubleClick() {}
        virtual void OnSaveCallback() {}
        virtual void CopiedInit() {}
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::AssetBase, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::AssetBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::Asset::AssetBase);