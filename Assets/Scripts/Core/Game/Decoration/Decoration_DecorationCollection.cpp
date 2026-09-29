#include "Decoration_DecorationCollection.h"

#include "Engine/Module/Guid/Guid.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace GameCore::Decoration
{
    DecorationCollection::DecorationCollection()
    {
        Reload();
    }

    void DecorationCollection::Reload()
    {
        data_ = NanamiEngine::Module::LocalPrefs::LoadOrDefaultWithPath(
            DECORATION_SAVE_FILE_PATH, DECORATION_SAVE_FILE_KEY, DecorationSaveData());
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    bool DecorationCollection::IsOwned(const Guid& decoration) const
    {
        return data_.owned.contains(decoration.Value());
    }

    bool DecorationCollection::Add(const Guid& decoration)
    {
        if (decoration.Value().empty() || !data_.owned.insert(decoration.Value()).second)
            return false;

        SaveAndNotify();
        return true;
    }

    bool DecorationCollection::Remove(const Guid& decoration)
    {
        if (data_.owned.erase(decoration.Value()) == 0)
            return false;

        SaveAndNotify();
        return true;
    }

    void DecorationCollection::SaveAndNotify()
    {
        NanamiEngine::Module::LocalPrefs::SaveWithPath(DECORATION_SAVE_FILE_PATH, DECORATION_SAVE_FILE_KEY, data_);
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    REGISTER_LOCAL_PREF_WITH_PATH(
        DecorationSaveData,
        DECORATION_SAVE_FILE_KEY,
        DecorationSaveData(),
        DECORATION_SAVE_FILE_PATH)
}
