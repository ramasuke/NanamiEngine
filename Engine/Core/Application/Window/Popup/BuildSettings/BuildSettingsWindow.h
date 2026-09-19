#pragma once
#include <string>

#include "../Interface/IPopupWindow.h"
#include "../Factory/PopupWindowFactory.h"

namespace NanamiEngine::Core::PopupWindow
{
    /** @brief Game 版のビルド設定 (製品名・起動シーン・構成・出力先) の編集とビルドの実行 */
    class BuildSettingsWindow final : public IPopupWindow
    {
    public:
        BuildSettingsWindow();
        ::Guid& Guid() override { return guid_; }
        PopupWindowState OnDraw(PopupWindowDrawGuiContext context) override;
        /** @brief 次に描くときに前面へ出す */
        void RequestFocus() { focusRequested_ = true; }

    private:
        /** @brief 編集中は入力途中の文字列を保ち、それ以外は保存済みの値を映す入力欄 */
        struct TextField
        {
            char buffer[512] = {};
            bool active      = false;
        };

        void OnDrawGameSettings();
        void OnDrawBuildSettings();
        void OnDrawActions();
        void OnDrawLastBuild();

        //NOTE: ImGUIのラベル情報のために現在開いているBuildSettingsWindowの数をカウントする
        static int counter_;
        int       id_;
        ::Guid    guid_;
        bool      focusRequested_ = true;
        TextField productName_;
        TextField outputDirectory_;
        TextField msBuildPath_;
    };

    REGISTER_POPUP_WINDOW(BuildSettingsWindow, "Build");
}
