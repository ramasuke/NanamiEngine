#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <string>
#include <vector>

// ゲーム DLL (Assets/Scripts) の読み込みと差し替え (docs/HotReload.md §5)。エディタ専用。
// Host exe が Run の前に LoadInitial し、以後は RequestReload -> 次のフレームの切れ目 (OnFrameEnd) で差し替える。
// DLL は HotReload/<世代>/ にコピーしてから読むので、リンカは元のファイルを上書きできる。
namespace NanamiEngine::Core::Application::HotReload
{
    class NANAMI_API GameModule final
    {
    public:
        static GameModule& Instance();

        /** @brief source (x64/Debug/<Product>.dll) を世代フォルダへコピーして読む。失敗なら false と理由 */
        bool LoadInitial(const std::filesystem::path& source, std::string& outError);
        void RequestReload();
        /** @brief ApplicationBase::Run が ScreenFlip の後に呼ぶ */
        void OnFrameEnd();

        [[nodiscard]] bool  IsLoaded     () const { return current_ != nullptr; }
        [[nodiscard]] void* CurrentModule() const { return current_; }
        [[nodiscard]] int   Generation   () const { return generation_; }
        [[nodiscard]] const std::filesystem::path& SourcePath() const { return source_; }
        [[nodiscard]] const std::string& LastReport() const { return lastReport_; }
        /** @brief true なら古い DLL を FreeLibrary しない (取り残しがあっても落ちない保険モード)。LocalPrefs に保存 */
        [[nodiscard]] bool KeepOldModules() const { return keepOldModules_; }
        void SetKeepOldModules(bool keep);

    private:
        GameModule();
        bool LoadGeneration(std::string& outError);
        void Reload();
        void ClearStagingRoot() const;

        std::filesystem::path source_;
        std::filesystem::path stagingRoot_;
        void*                 current_         = nullptr;
        std::vector<void*>    retired_;
        int                   generation_      = 0;
        bool                  reloadRequested_ = false;
        bool                  keepOldModules_  = true;
        std::string           lastReport_;
    };
}
