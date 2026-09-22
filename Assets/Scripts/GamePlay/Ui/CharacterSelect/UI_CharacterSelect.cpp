#include "UI_CharacterSelect.h"

#include <iterator>

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Ui
{
    void CharacterSelectUi::BuildRoster(const std::vector<std::shared_ptr<Asset::CharacterData>>& characters)
    {
        if (!rows_.empty() || !rowPrefab_ || !rowsRoot_)
            return;

        const auto rowsObject = rowsRoot_.get();
        for (size_t i = 0; i < characters.size(); ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab_.get(), rowsObject).lock();
            if (!rowObject)
                continue;

            rowObject->Transform().SetLocalPos(glm::vec3(0.0f, static_cast<float>(i) * rowSpacing_px_, 0.0f));

            const auto row = rowObject->Components().Catch<CharacterSelectRow>();
            if (const auto locked = row.lock())
                locked->Initialize(characters[i]);
            rows_.push_back(row);
        }
    }

    void CharacterSelectUi::HighlightRow(const size_t index) const
    {
        for (size_t i = 0; i < rows_.size(); ++i)
        {
            if (const auto row = rows_[i].lock())
                row->SetHighlighted(i == index);
        }
    }

    void CharacterSelectUi::ShowDetail(const Asset::CharacterData& character) const
    {
        detailNameText_   ->SetText(character.DisplayName());
        detailReadingText_->SetText(character.Reading());

        const int pips[] = { character.PowerPips(), character.ToughnessPips(), character.AgilityPips() };
        for (size_t i = 0; i < detailStatPips_.size(); ++i)
        {
            if (const auto strip = detailStatPips_[i].get())
                strip->SetDifficulty(i < std::size(pips) ? pips[i] : 0);
        }

        const auto& lines = character.DescriptionLines();
        for (size_t i = 0; i < detailDescriptionLines_.size(); ++i)
        {
            const auto text = detailDescriptionLines_[i].get();
            if (!text)
                continue;

            const bool hasLine = i < lines.size();
            text->SetEnable(hasLine);
            if (hasLine)
                text->SetText(lines[i]);
        }
    }

    void CharacterSelectUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("detailNameText_", detailNameText_);
        ImGuiHelper::OnDrawInputField("detailReadingText_", detailReadingText_);
        ImGuiHelper::OnDrawInputField("detailStatPips_", detailStatPips_, [this]
        {
            if (ImGui::Button("Add Stat"))
            {
                detailStatPips_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("detailDescriptionLines_", detailDescriptionLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                detailDescriptionLines_.emplace_back();
            }
        });
    }
}
