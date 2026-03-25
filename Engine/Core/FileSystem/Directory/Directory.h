#pragma once
#include <string>
#include <vector>

#include "../../Object/IObject.h"
#include "../File/File.h"

namespace NanamiEngine::Core::FileSystem
{
    class Directory final
    {
    public:
        explicit Directory(const std::string& ownPath);
        void OnSave();
        void AddFile(const File& file);
        Directory& AddDirectory();


        [[nodiscard]] const std::string& GetPath() const       { return ownPath_;     }
        [[nodiscard]] const std::string& GetName() const       { return name_;        }
        [[nodiscard]] std::vector<Directory>& GetDirectories() { return directories_; }
        [[nodiscard]] std::vector<File>& Files()               { return files_;       }

    private:
        std::string ownPath_;
        std::string name_;
        std::vector<Directory> directories_;
        std::vector<File> files_;
    };
}