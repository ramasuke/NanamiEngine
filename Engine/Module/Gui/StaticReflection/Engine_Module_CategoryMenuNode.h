#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Engine_Module_CategoryMenuItem.h"

namespace NanamiEngine::Module::StaticReflection
{
    struct NANAMI_NO_API CategoryMenuNode
    {
        std::map<std::string, std::unique_ptr<CategoryMenuNode>> children;
        std::vector<const CategoryMenuItem*>                     items;
    };
}
