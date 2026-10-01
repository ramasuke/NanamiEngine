#include "Ui_SettingsScreen.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        using LibCore::EaseType;
        using LibCore::Tween::Ease;
        using LibCore::Tween::Ms;
    }

    void SettingsScreenUi::BuildTabs(const std::vector<std::string>& names)
    {
        if (!tabs_.empty() || !tabPrefab_ || !tabsRoot_)
            return;

        const auto tabsObject = tabsRoot_.get();
        for (size_t i = 0; i < names.size(); ++i)
        {
            const auto tabObject = Scene::GameObject::Instantiate(*tabPrefab_.get(), tabsObject).lock();
            if (!tabObject)
                continue;

            tabObject->Transform().SetLocalPos(glm::vec3(0.0f, static_cast<float>(i) * tabPitch_px_, 0.0f));
            const auto tab = tabObject->Components().Catch<SettingsTabUi>();
            if (const auto locked = tab.lock())
                locked->Show(names[i]);
            tabs_.push_back(tab);
        }
    }

    void SettingsScreenUi::SetTabSelection(const size_t index) const
    {
        for (size_t i = 0; i < tabs_.size(); ++i)
        {
            if (const auto tab = tabs_[i].lock())
                tab->SetSelected(i == index);
        }
    }

    void SettingsScreenUi::SetCategoryName(const std::string& name) const
    {
        if (const auto text = categoryText_.get())
            text->SetText(name);
    }

    size_t SettingsScreenUi::VisibleRowCount()
    {
        EnsureRows();
        return rows_.size();
    }

    void SettingsScreenUi::EnsureRows()
    {
        if (!rows_.empty() || !rowPrefab_ || !rowsRoot_)
            return;

        const auto rowsObject = rowsRoot_.get();
        for (int i = 0; i < maxVisibleRows_; ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab_.get(), rowsObject).lock();
            if (!rowObject)
                continue;

            rowObject->Transform().SetLocalPos(glm::vec3(0.0f, static_cast<float>(i) * rowPitch_px_, 0.0f));
            rows_.push_back(rowObject->Components().Catch<SettingsRowUi>());
        }
    }

    void SettingsScreenUi::SetRow(const size_t slot, const std::string& label, const std::string& value, const bool isSelected)
    {
        EnsureRows();
        if (slot >= rows_.size())
            return;

        const auto row = rows_[slot].lock();
        if (!row)
            return;

        row->Show(label, value);
        row->SetSelected(isSelected && !label.empty());
    }

    void SettingsScreenUi::SetScroll(const size_t firstVisibleIndex, const size_t count)
    {
        const size_t visible = rows_.size();
        const bool isOverflowing = count > visible;
        if (const auto track = scrollTrack_.get())
            track->SetEnable(isOverflowing);

        const auto thumb = scrollThumb_.get();
        if (!thumb)
            return;

        thumb->SetEnable(isOverflowing);
        if (!isThumbBaseTaken_)
        {
            thumbBasePos_ = thumb->Transform().GetLocalPos();
            isThumbBaseTaken_ = true;
        }
        if (!isOverflowing)
            return;

        const float rate = static_cast<float>(firstVisibleIndex) / static_cast<float>(count - visible);
        thumb->Transform().SetLocalPos(thumbBasePos_ + glm::vec3(0.0f, scrollTravel_px_ * std::clamp(rate, 0.0f, 1.0f), 0.0f));
    }

    void SettingsScreenUi::SetDescription(const std::string& text) const
    {
        if (const auto description = descriptionText_.get())
            description->SetText(text);
    }

    void SettingsScreenUi::PlayEnter()
    {
        const auto root = visualRoot_.get();
        if (!root)
            return;

        visualBasePos_ = root->Transform().GetLocalPos();
        enterTween_.Play(tweeny::from(0.0f).to(1.0f).during(Ms(enterDuration_secs_)).via(Ease(EaseType::OutCubic)));
        root->Transform().SetLocalPos(visualBasePos_ + glm::vec3(0.0f, enterSlide_px_, 0.0f));
    }

    void SettingsScreenUi::OnUpdate()
    {
        if (!enterTween_.IsPlaying())
            return;

        enterTween_.Tick(Time::DeltaTime());
        if (const auto root = visualRoot_.get())
            root->Transform().SetLocalPos(visualBasePos_ + glm::vec3(0.0f, enterSlide_px_ * (1.0f - enterTween_.Value()), 0.0f));
    }

    void SettingsScreenUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("visualRoot_", visualRoot_);
        ImGuiHelper::OnDrawInputField("tabPrefab_", tabPrefab_);
        ImGuiHelper::OnDrawInputField("tabsRoot_", tabsRoot_);
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("categoryText_", categoryText_);
        ImGuiHelper::OnDrawInputField("descriptionText_", descriptionText_);
        ImGuiHelper::OnDrawInputField("scrollTrack_", scrollTrack_);
        ImGuiHelper::OnDrawInputField("scrollThumb_", scrollThumb_);
        ImGuiHelper::OnDrawInputField("tabPitch_px_", tabPitch_px_);
        ImGuiHelper::OnDrawInputField("rowPitch_px_", rowPitch_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("scrollTravel_px_", scrollTravel_px_);
        ImGuiHelper::OnDrawInputField("enterSlide_px_", enterSlide_px_);
        ImGuiHelper::OnDrawInputField("enterDuration_secs_", enterDuration_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SettingsScreenUi);
#pragma endregion
