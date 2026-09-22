#pragma once
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../StageSelect/Difficulty/StageDifficultyPips.h"
#include "Row/Ui_CharacterSelect_Row.h"

namespace GamePlay::Ui
{
    /**
     * @brief 酒場のキャラ選択画面の見た目。手配書の行は prefab から並べ、右の帳面に選択中の詳細を出す。
     * キャラの3Dは画面に描かず、展示台(Prop::CharacterPodium)の実モデルが担当する。
     */
    class CharacterSelectUi final : public Component::ComponentBase
    {
    public:
        void BuildRoster(const std::vector<std::shared_ptr<Asset::CharacterData>>& characters);
        [[nodiscard]] const std::vector<std::weak_ptr<CharacterSelectRow>>& Rows() const { return rows_; }

        void HighlightRow(size_t index) const;
        void ShowDetail  (const Asset::CharacterData& character) const;

    private:
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] float rowSpacing_px_ = 204.0f;

        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailNameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) detailReadingText_;
        // 腕っぷし / しぶとさ / 身軽さ の順に並べる
        [[serialize(0)]] std::vector<FIELD(StageDifficultyPips)> detailStatPips_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> detailDescriptionLines_;

        std::vector<std::weak_ptr<CharacterSelectRow>> rows_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(rowsRoot_));
            archive(CEREAL_NVP(rowSpacing_px_));
            archive(CEREAL_NVP(detailNameText_));
            archive(CEREAL_NVP(detailReadingText_));
            archive(CEREAL_NVP(detailStatPips_));
            archive(CEREAL_NVP(detailDescriptionLines_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(rowsRoot_));
            if (version >= 0) archive(CEREAL_NVP(rowSpacing_px_));
            if (version >= 0) archive(CEREAL_NVP(detailNameText_));
            if (version >= 0) archive(CEREAL_NVP(detailReadingText_));
            if (version >= 0) archive(CEREAL_NVP(detailStatPips_));
            if (version >= 0) archive(CEREAL_NVP(detailDescriptionLines_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::CharacterSelectUi, 0)
