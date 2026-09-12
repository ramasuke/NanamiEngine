#include "StageSelectModel.h"

namespace GamePlay::Ui
{
    StageSelectModel::StageSelectModel(std::vector<std::weak_ptr<StageSelectStageUi>> stages)
        : stages_(std::move(stages))
    {
    }

    void StageSelectModel::SelectStage(const size_t index)
    {
        if (index >= stages_.size())
            return;

        selectedIndex_ = index;
        hasSelection_ = true;
        onSelectionChanged_.get_subscriber().on_next(index);
    }

    GameCore::Scene::Main::SceneType StageSelectModel::SelectedSceneType() const
    {
        return stages_[selectedIndex_].lock()->SceneType();
    }
}
