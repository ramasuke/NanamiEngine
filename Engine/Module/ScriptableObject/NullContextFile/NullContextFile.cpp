#include "NullContextFile.h"
#include <filesystem>
#include <fstream>

namespace NanamiEngine::Module::Scriptable
{
    NullContextFile::NullContextFile(std::string filePath)
        : filePath_(std::move(filePath))
    {
    }

    void NullContextFile::OnSave() const
    {
        if (filePath_.empty())
            return;

        std::filesystem::create_directories(std::filesystem::path(filePath_).parent_path());
        std::ofstream ofs(filePath_, std::ios::trunc);
    }

    void NullContextFile::OnDrawGui()
    {
    }
}
