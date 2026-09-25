#include "Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace NanamiEngine::Module::LocalPrefs::Editor
{
    LocalPrefsRegistry& LocalPrefsRegistry::GetInstance()
    {
        static LocalPrefsRegistry instance;
        return instance;
    }

    void LocalPrefsRegistry::Register(PrefInfo info)
    {
        m_prefsList.push_back(std::move(info));
    }

    std::size_t LocalPrefsRegistry::UnregisterModule(const void* module)
    {
        return std::erase_if(m_prefsList, [module](const PrefInfo& info) { return info.module == module; });
    }

    const std::vector<LocalPrefsRegistry::PrefInfo>& LocalPrefsRegistry::GetPrefsList() const
    {
        return m_prefsList;
    }
}
