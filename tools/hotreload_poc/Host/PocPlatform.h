#pragma once
// Host が Game.dll を出し入れするための最小のラッパー。Windows が本番、Linux は g++ + dlopen による機構の確認用
// (tools/hotreload_poc/emulate_linux.sh)。
#include <string>

#if defined(_WIN32)
#include <Windows.h>
namespace Poc::Platform
{
    using ModuleHandle = HMODULE;
    inline const char* LibraryExtension() { return ".dll"; }
    inline bool CopyFileTo(const std::string& from, const std::string& to) { return CopyFileA(from.c_str(), to.c_str(), FALSE) != 0; }
    inline void DeleteFileAt(const std::string& path) { DeleteFileA(path.c_str()); }
    inline ModuleHandle Load(const std::string& path) { return LoadLibraryA(path.c_str()); }
    inline void* Symbol(ModuleHandle module, const char* name) { return reinterpret_cast<void*>(GetProcAddress(module, name)); }
    inline bool Unload(ModuleHandle module) { return FreeLibrary(module) != 0; }
    inline std::string LastError() { return std::to_string(GetLastError()); }
    // exe のあるフォルダ (末尾の区切り付き)。DLL はカレントディレクトリではなく exe の隣から読む
    inline std::string ExecutableDirectory()
    {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string dir(path);
        const auto slash = dir.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : dir.substr(0, slash + 1);
    }
    inline bool IsAbsolute(const std::string& path) { return path.size() > 1 && (path[1] == ':' || path[0] == '\\' || path[0] == '/'); }
}
#else
#include <dlfcn.h>
#include <unistd.h>
#include <fstream>
#include <cstdio>
namespace Poc::Platform
{
    using ModuleHandle = void*;
    inline const char* LibraryExtension() { return ".so"; }
    inline bool CopyFileTo(const std::string& from, const std::string& to)
    {
        std::ifstream in(from, std::ios::binary); std::ofstream out(to, std::ios::binary);
        out << in.rdbuf();
        return in.good() && out.good();
    }
    inline void DeleteFileAt(const std::string& path) { std::remove(path.c_str()); }
    // RTLD_DEEPBIND: Windows と同じく、DLL 内の参照は自分のシンボルを優先する (テンプレートの static がモジュールごとになる)
    inline ModuleHandle Load(const std::string& path) { return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND); }
    inline void* Symbol(ModuleHandle module, const char* name) { return dlsym(module, name); }
    inline bool Unload(ModuleHandle module) { return dlclose(module) == 0; }
    inline std::string LastError() { const char* e = dlerror(); return e ? e : ""; }
    inline std::string ExecutableDirectory()
    {
        char path[4096] = {};
        const auto n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        std::string dir(path, n > 0 ? static_cast<size_t>(n) : 0);
        const auto slash = dir.find_last_of('/');
        return slash == std::string::npos ? std::string() : dir.substr(0, slash + 1);
    }
    inline bool IsAbsolute(const std::string& path) { return !path.empty() && path[0] == '/'; }
}
#endif
