#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../../Libs/Singleton/LibCore_SingletonBase.h"

namespace NanamiEngine::DebugSheet
{
    /**
     * @brief UnityDebugSheet 風のデバッグメニュー。ページを "Save/Reset All" のようなパスで登録し、
     *        カテゴリ一覧 -> 子ページとページスタックで辿る
     * @note  エンジンからは呼ばれない。ゲーム側が毎フレーム Update() と、UI 描画の最後に Render() を呼ぶ
     */
    class NANAMI_API Sheet final : public SingletonBase<Sheet>
    {
    public:
        /** @brief SingletonBase<T>::Instance() はテンプレートなのでモジュール (exe / DLL) ごとに実体が分かれる。1 つにするため .cpp で定義する (docs/HotReload.md §3.1) */
        static Sheet& Instance();

        friend class SingletonBase<Sheet>;

    public:
        using DrawPage = std::function<void()>;

        void RegisterPage(const std::string& path, DrawPage draw, int order = 0);
        /** @param module 登録元のモジュール (REGISTER_DEBUG_SHEET_PAGE が NANAMI_CURRENT_MODULE() を渡す) */
        void RegisterPage(const std::string& path, DrawPage draw, int order, Core::ModuleHandle module);
        /** @brief module が登録したページを消す。空になったカテゴリも消す。戻り値は消したページ数 */
        std::size_t UnregisterModule(Core::ModuleHandle module);

        /**
         * @brief F1 で開閉する
         * @note  ゲーム実行中（エディタではプレイ中・一時停止中）だけ。プレイを終えると閉じる
         */
        void Update();
        /**
         * @brief 開いていれば描く
         * @note  エディタでは ImGui のフレーム中に呼ばれる前提。ゲームビルドでは ImGui を自前で用意してフレームを回す
         */
        void Render();

        /** @brief ゲーム実行中でなければ何もしない */
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
            /** ページ (draw があるノード) の登録元モジュール */
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
