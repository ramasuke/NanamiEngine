#pragma once
#include <string>

#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Scriptable
{
    class NullContextFile final : public Object::IObject
    {
    public:
        explicit NullContextFile(std::string filePath = "");
        ~NullContextFile() override = default;

        [[nodiscard]]
        const Guid& GetGuid() const override { return guid_; }
        void OnSave() const;
        void OnDrawGui() override;

    private:
        std::string filePath_;
        Guid guid_;
    };
}
