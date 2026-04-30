#include "NanamiEngine_Module_Log.h"

#include <iostream>
#include <__msvc_ostream.hpp>

namespace NanamiEngine::Module
{
    void Log(const std::string& text)
    {
        std::cout << text << '\n';
    }

    void LogWarning(const std::string& text)
    {
        std::cout << text << '\n';
    }

    void LogError(const std::string& text)
    {
        std::cerr << text << '\n';
    }
}
