#pragma once
#include <string>

namespace NanamiEngine::Module
{
    void Log       (const std::string& text);
    void LogWarning(const std::string& text);
    void LogError  (const std::string& text);
}
