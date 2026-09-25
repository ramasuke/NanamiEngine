#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "GraphEditor.h"
#include "../../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Gui::Graph
{
    /**
     * @brief ImGuizmo の GraphEditor を 1 枚のキャンバスとして描画するホスト。
     * @note  ツールバー（Fit / Grid / Minimap / 曲線）と ViewState（パン・ズーム）を持ち、
     *        ノードの中身は GraphEditor::Delegate 側が提供する。ViewState はセッション中だけ保持し、保存しない。
     *        操作: 中ボタンドラッグ = パン / ホイール = ズーム / 左ドラッグ = 範囲選択 / F = 全体表示
     */
    class NANAMI_API GraphEditorHost final
    {
    public:
        GraphEditorHost();

        /**
         * @brief ツールバーとキャンバスを現在のウィンドウの残り領域に描画する
         * @param readOnly true ならノードの移動・リンク編集を禁止する（実行中ツリーの表示用）
         */
        void Draw(GraphEditor::Delegate& delegate, bool readOnly);

        /** @brief スクリーン座標を、ノード位置と同じグラフ座標に変換する（直前の Draw 時点の表示で計算） */
        [[nodiscard]] ImVec2 ScreenToGraph(const ImVec2& screenPosition) const;

        /** @brief 次の Draw で全ノード（またはselectedOnly なら選択中ノード）が収まるように表示を合わせる */
        void RequestFit(bool selectedOnly = false);

        /** @brief キャンバスのウィンドウ（子ウィンドウ含む）がフォーカスされているか。キーボードショートカットの判定用 */
        [[nodiscard]] bool IsFocused() const { return isFocused_; }

        [[nodiscard]] GraphEditor::Options& Options() { return options_; }

    private:
        void DrawToolbar();

        GraphEditor::Options     options_;
        GraphEditor::ViewState   viewState_;
        GraphEditor::FitOnScreen fit_ = GraphEditor::Fit_AllNodes;
        ImVec2                   canvasOrigin_ = ImVec2(0, 0);
        bool                     showMinimap_  = true;
        bool                     isFocused_    = false;
    };

    /** @brief ノードの見た目テンプレートを作る（入出力スロット名は持たない） */
    [[nodiscard]] NANAMI_API GraphEditor::Template MakeNodeTemplate(ImU32 headerColor, ImU8 inputCount, ImU8 outputCount);

    /** @brief 開いている全 InspectorWindow に対象を表示させる */
    NANAMI_API void ShowInInspector(const std::weak_ptr<Object::IObject>& object);
}
