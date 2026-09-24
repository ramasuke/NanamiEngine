#pragma once
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
    class Sheet final : public SingletonBase<Sheet>
    {
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
        struct Node
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
