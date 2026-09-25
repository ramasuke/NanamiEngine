// Linux で PoC の機構だけを確かめるための Windows.h スタブ (tools/hotreload_poc/emulate_linux.sh から使う)。
// GetModuleHandleExW(FROM_ADDRESS) を dladdr で真似る: モジュールの識別子はロードイメージの先頭アドレス。
#pragma once
#include <dlfcn.h>

typedef struct HINSTANCE__* HMODULE;
typedef const wchar_t*      LPCWSTR;
typedef unsigned long       DWORD;
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS      0x4
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x2

inline int GetModuleHandleExW(DWORD, LPCWSTR address, HMODULE* out)
{
    Dl_info info{};
    if (dladdr(reinterpret_cast<const void*>(address), &info) == 0 || info.dli_fbase == nullptr)
    {
        *out = nullptr;
        return 0;
    }
    *out = static_cast<HMODULE>(info.dli_fbase);
    return 1;
}
