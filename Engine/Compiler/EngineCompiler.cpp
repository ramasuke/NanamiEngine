#include "EngineCompiler.h"

#include <algorithm>

[[nodiscard]] int NanamiEngine::Compiler::EngineCompiler::GetMaxVersion(const std::vector<Member>& members)
{
    int maxVersion = 0;
    for (const auto& m : members)
    {
        maxVersion = std::max(m.version_, maxVersion);
    }
    return maxVersion;
}
