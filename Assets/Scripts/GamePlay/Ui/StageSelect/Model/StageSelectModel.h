#pragma once
#include <vector>
#include "../Stage/Ui_StageSelect_StageUI.h"

namespace GamePlay::Ui
{
    // NOTE: ステージ選択画面の Model。選択中のステージだけを持つ
    class StageSelectModel final
    {
    public:
        explicit StageSelectModel(std::vector<std::weak_ptr<StageSelectStageUi>> stages);

        [[nodiscard]] const std::vector<std::weak_ptr<StageSelectStageUi>>& Stages() const { return stages_; }

        void SelectStage(size_t index);

        [[nodiscard]] bool   HasSelection () const { return hasSelection_; }
        [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
        [[nodiscard]] GameCore::Scene::Main::SceneType SelectedSceneType() const;
        [[nodiscard]] std::shared_ptr<Asset::StageData> SelectedStageData() const;

        [[nodiscard]] NanamiEngine::R4::Observable<size_t> OnSelectionChanged() const { return onSelectionChanged_.AsObservable(); }

    private:
        std::vector<std::weak_ptr<StageSelectStageUi>> stages_;
        bool hasSelection_ = false;
        size_t selectedIndex_ = 0;
        NanamiEngine::R4::Subject<size_t> onSelectionChanged_;
    };
}
