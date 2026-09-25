#include "AsyncLoad.h"

#include "DxLib.h"

namespace NanamiEngine::Platform::AsyncLoad
{
    SyncLoadScope::SyncLoadScope()
        : wasAsync_(GetUseASyncLoadFlag() != FALSE)
    {
        SetUseASyncLoadFlag(FALSE);
    }

    SyncLoadScope::~SyncLoadScope()
    {
        SetUseASyncLoadFlag(wasAsync_ ? TRUE : FALSE);
    }

    bool IsEnabled()
    {
        return GetUseASyncLoadFlag() != FALSE;
    }

    bool IsHandleLoading(const int handle)
    {
        return CheckHandleASyncLoad(handle) == TRUE;
    }
}
