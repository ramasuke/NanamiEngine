#pragma once
#include <memory>
#include <vector>

#include "../Row/Ui_CharacterSelect_Row.h"

namespace GamePlay::Ui
{
    /**
     * 酒場のキャラ選択のModel。どのキャラに寄っているかという状態だけを持つ、
     * プレーンなランタイムクラス(アセットでもコンポーネントでもない)。
     */
    class CharacterSelectModel final
    {
    public:
        explicit CharacterSelectModel(std::vector<std::shared_ptr<Asset::CharacterData>> characters);

        [[nodiscard]] const std::vector<std::shared_ptr<Asset::CharacterData>>& Characters() const { return characters_; }
        [[nodiscard]] size_t SelectedIndex() const { return selectedIndex_; }
        [[nodiscard]] std::shared_ptr<Asset::CharacterData> Selected() const;
        /** @brief 未解放のキャラには切り替えられない */
        [[nodiscard]] bool CanConfirm() const;

        void Select(size_t index);
        /** @brief 端で止める。名簿は短いので巡回させない */
        void MoveSelection(int delta);

        [[nodiscard]] rxcpp::observable<size_t> OnSelectionChanged() const { return onSelectionChanged_.get_observable(); }

    private:
        std::vector<std::shared_ptr<Asset::CharacterData>> characters_;
        size_t selectedIndex_ = 0;
        rxcpp::subjects::subject<size_t> onSelectionChanged_;
    };
}
