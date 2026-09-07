#include "Engine_Module_SafeExecute.h"

#include <Windows.h>
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <atomic>
#include <cstring>
#include <malloc.h>
#include <mutex>
#include <sstream>

#include "../Exception/Engine_Module_Exception.h"
#include "../Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Module
{
    namespace
    {
        // MSVCがC++のthrowをSEH上に実装する際に使う既知のコード。
        // このコードのときは中身がC++例外なので、ここでは触らず外側のtry/catchへ素通りさせる。
        constexpr unsigned long kCxxExceptionCode = 0xE06D7363;

        std::atomic<bool>& CrashRecoveryEnabledFlag()
        {
            static std::atomic enabled{false};
            return enabled;
        }

        std::atomic<bool>& DebuggerFailFastEnabledFlag()
        {
            static std::atomic enabled{true}; // デフォルトON
            return enabled;
        }

        std::string DescribeSehCode(const unsigned long code)
        {
            switch (code)
            {
            case EXCEPTION_ACCESS_VIOLATION:      return "EXCEPTION_ACCESS_VIOLATION (nullptr等への不正アクセス)";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "EXCEPTION_INT_DIVIDE_BY_ZERO (0除算)";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (配列範囲外アクセス)";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:    return "EXCEPTION_FLT_DIVIDE_BY_ZERO (浮動小数点0除算)";
            case EXCEPTION_ILLEGAL_INSTRUCTION:   return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_STACK_OVERFLOW:        return "EXCEPTION_STACK_OVERFLOW";
            default:
                {
                    std::ostringstream oss;
                    oss << "SEH exception 0x" << std::hex << code;
                    return oss.str();
                }
            }
        }

        std::mutex& SymMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        // プロセス内で一度だけ初期化する(以降のSym*呼び出しはSymMutex()で必ず保護すること。
        bool EnsureSymbolsInitialized()
        {
            static const bool initialized = []()
            {
                SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
                return SymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE;
            }();
            return initialized;
        }
        
        std::string BaseFileName(const char* fullPath)
        {
            if (fullPath == nullptr) return "";
            const char* base = fullPath;
            if (const char* p = strrchr(fullPath, '\\'); p) base = p + 1;
            if (const char* p = strrchr(base,     '/');  p) base = p + 1;
            return base;
        }

        // SEHフィルタ式から(=unwind前に)呼ぶ想定。ロック・std::string・ostringstream
        void AppendStackTrace(const CONTEXT& contextAtFault, std::string& outStackTrace)
        {
            std::lock_guard lock(SymMutex());
            const bool symbolsReady = EnsureSymbolsInitialized();
            const HANDLE process = GetCurrentProcess();
            const HANDLE thread  = GetCurrentThread();

            CONTEXT context = contextAtFault;
            STACKFRAME64 frame{};
            frame.AddrPC.Mode    = AddrModeFlat;
            frame.AddrFrame.Mode = AddrModeFlat;
            frame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_X64)
            frame.AddrPC.Offset    = context.Rip;
            frame.AddrFrame.Offset = context.Rbp;
            frame.AddrStack.Offset = context.Rsp;
            constexpr DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
            frame.AddrPC.Offset    = context.Eip;
            frame.AddrFrame.Offset = context.Ebp;
            frame.AddrStack.Offset = context.Esp;
            constexpr DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif
            std::ostringstream oss;
            constexpr int kMaxFrames = 32;
            for (int i = 0; i < kMaxFrames; ++i)
            {
                if (!StackWalk64(machineType, process, thread, &frame, &context, nullptr,
                                  SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                    break;
                if (frame.AddrPC.Offset == 0)
                    break;

                oss << "  #" << i << " 0x" << std::hex << frame.AddrPC.Offset << std::dec;

                if (symbolsReady)
                {
                    alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
                    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
                    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                    symbol->MaxNameLen   = MAX_SYM_NAME;

                    DWORD64 symDisplacement = 0;
                    if (SymFromAddr(process, frame.AddrPC.Offset, &symDisplacement, symbol))
                    {
                        oss << " " << symbol->Name << "+0x" << std::hex << symDisplacement << std::dec;

                        DWORD lineDisplacement = 0;
                        IMAGEHLP_LINE64 line{};
                        line.SizeOfStruct = sizeof(line);
                        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
                            oss << " (" << BaseFileName(line.FileName) << ":" << line.LineNumber << ")";
                    }
                }
                oss << "\n";
            }
            outStackTrace += oss.str();
        }

        bool InvokeGuardedSEH(const std::function<void()>& func, unsigned long& outSehCode, std::string& outStackTrace)
        {
            __try
            {
                func();
                return true;
            }
            __except (GetExceptionCode() == kCxxExceptionCode
                       || !CrashRecoveryEnabledFlag().load(std::memory_order_relaxed)
                       || (DebuggerFailFastEnabledFlag().load(std::memory_order_relaxed) && IsDebuggerPresent())
                       ? EXCEPTION_CONTINUE_SEARCH // C++例外、トグルOFF、またはデバッガアタッチ中なら素通り(=fail-fast)
                       : (outSehCode = GetExceptionCode(),
                          AppendStackTrace(*GetExceptionInformation()->ContextRecord, outStackTrace),
                          EXCEPTION_EXECUTE_HANDLER))
            {
                return false;
            }
        }
    }

    bool IsCrashRecoveryEnabled()
    {
        return CrashRecoveryEnabledFlag().load(std::memory_order_relaxed);
    }

    void SetCrashRecoveryEnabled(const bool enabled)
    {
        CrashRecoveryEnabledFlag().store(enabled, std::memory_order_relaxed);
    }

    bool IsDebuggerFailFastEnabled()
    {
        return DebuggerFailFastEnabledFlag().load(std::memory_order_relaxed);
    }

    void SetDebuggerFailFastEnabled(const bool enabled)
    {
        DebuggerFailFastEnabledFlag().store(enabled, std::memory_order_relaxed);
    }

    bool SafeExecute(const std::function<void()>& func, std::string& outErrorMessage)
    {
        unsigned long sehCode = 0;
        std::string stackTrace;
        bool sehOk;
        try
        {
            // ここで NanamiException/std::exception が起きた場合、InvokeGuardedSEH内の
            // __except は上のフィルタで素通しするので、例外はそのままここまで伝播してくる。
            sehOk = InvokeGuardedSEH(func, sehCode, stackTrace);
        }
        catch (const Exception::NanamiException& e)
        {
            outErrorMessage = std::string("NanamiException: ") + e.what();
            return false;
        }
        catch (const std::exception& e)
        {
            outErrorMessage = std::string("std::exception: ") + e.what();
            return false;
        }
        catch (...)
        {
            outErrorMessage = "unknown C++ exception";
            return false;
        }

        if (sehOk)
            return true;

        if (sehCode == EXCEPTION_STACK_OVERFLOW)
        {
            _resetstkoflw();
            LogError("[FATAL] EXCEPTION_STACK_OVERFLOW detected. Cannot safely continue; terminating process.");
            ExitProcess(1);
        }

        outErrorMessage = DescribeSehCode(sehCode);
        if (!stackTrace.empty())
            outErrorMessage += "\n" + stackTrace;
        return false;
    }
}
