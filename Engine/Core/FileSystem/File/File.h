#pragma once
#include <memory>
#include <string>

#include "../../../Module/Asset/AssetBase.h"

namespace NanamiEngine::Core::FileSystem
{
    class File final
    {
    public:
        [[nodiscard]] static File OnLoadForMeta(const std::string& filePath, std::string fileName);
        [[nodiscard]] static File CreateOrLoad(std::string filePath, std::string fileName);

        [[nodiscard]] const std::string& GetName() const { return fileName_; }
        [[nodiscard]] const std::shared_ptr<Module::Asset::AssetBase>& GetContent() const { return content_; }
        [[nodiscard]] File Copy() const;

        void OnSave()        const;
        void OnClick()       const;
        void OnDoubleClick() const;

    private:
        explicit File() = default;
        
        std::string filePath_;
        std::string fileName_;
        std::shared_ptr<Module::Asset::AssetBase> content_;
    };
}
