#pragma once
#include "../DebugSheetConfig.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace NanamiEngine::DebugSheet
{
    // NOTE: デバッグメニュー。ページを "Save/Reset All" のようなパスで登録し、ページスタックで辿る
    class NANAMI_API Sheet final : public SingletonBase<Sheet>
    {
    public:
        // NOTE: モジュール (exe / DLL) ごとに実体が分かれないよう .cpp で定義する (docs/HotReload.md §3.1)
        static Sheet& Instance();

        friend class SingletonBase<Sheet>;

    public:
        using DrawPage = std::function<void()>;

        void RegisterPage(const std::string& path, DrawPage draw, int order = 0);
        // NOTE: module は登録元のモジュール。ホットリロード時にまとめて解除するために記録する
        void RegisterPage(const std::string& path, DrawPage draw, int order, Core::ModuleHandle module);
        // NOTE: module が登録したページを消す。空になったカテゴリも消す。戻り値は消したページ数
        std::size_t UnregisterModule(Core::ModuleHandle module);

        // NOTE: F1 で開閉する
        // NOTE: ゲーム実行中（エディタではプレイ中・一時停止中）だけ。プレイを終えると閉じる
        void Update();
        // NOTE: エディタでは ImGui のフレーム中に呼ばれる前提。ゲームビルドでは ImGui を自前で用意してフレームを回す
        void Render();

        // NOTE: ゲーム実行中でなければ何もしない
        void Open();
        void Close();
        void Toggle();
        [[nodiscard]] bool IsOpen() const { return isOpen_; }

    private:
        // NOTE: unique_ptr の vector を持つ集成体。export すると暗黙のコピーが実体化されて壊れるので export しない
        struct NANAMI_NO_API Node
        {
            std::string                        name;
            int                                order = 0;
            DrawPage                           draw;
            // NOTE: ページ (draw があるノード) の登録元モジュール
            Core::ModuleHandle                 module;
            std::vector<std::unique_ptr<Node>> children;
        };

        static std::size_t RemovePagesOfModule(Node& node, Core::ModuleHandle module);

        Sheet();

        void DrawWindow();
        void DrawHeader();
        void DrawNode(Node& node);
        void SortChildren(Node& node);
        [[nodiscard]] Node& Current() const;

        Node               root_;
        std::vector<Node*> stack_;
        bool               isOpen_          = false;
        bool               toggleKeyHeld_   = false;
        bool               isSortDirty_     = false;
        bool               isImGuiReady_    = false;
    };
}
#endif
