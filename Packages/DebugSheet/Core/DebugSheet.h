#pragma once
#include "Engine/Core/Api/NanamiApi.h"
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

        /** @brief F1 で開閉する */
        void Update();
        /**
         * @brief 開いていれば描く
         * @note  エディタでは ImGui のフレーム中に呼ばれる前提。ゲームビルドでは ImGui を自前で用意してフレームを回す
         */
        void Render();

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
            std::vector<std::unique_ptr<Node>> children;
        };

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
