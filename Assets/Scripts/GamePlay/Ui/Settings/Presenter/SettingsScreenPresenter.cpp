#include "SettingsScreenPresenter.h"

#include <algorithm>

#include "Assets/Scripts/Core/Input/InputAliases.h"
#include "../Ui_SettingsScreen.h"
#include "../Model/SettingsCatalog.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/Settings/GameSettings.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short SETTINGS_STICK_DEADZONE = 12000;
    }

    std::weak_ptr<SettingsScreenPresenter> SettingsScreenPresenter::Open(Asset::PrefabGameObjectFile& prefab)
    {
        if (isOpen_)
            return {};

        // UIは world 座標がそのままスクリーン座標
        const auto ui = Scene::GameObject::Instantiate(prefab, glm::vec3(0.0f, 0.0f, 0.0f)).lock();
        if (!ui)
            return {};
        return ui->Components().Catch<SettingsScreenPresenter>();
    }

    void SettingsScreenPresenter::OnStart()
    {
        if (isOpen_)
        {
            isClosed_ = true;
            Entity().lock()->OnDestroy();
            return;
        }
        isOpen_  = true;
        isOwner_ = true;

        view_ = RequireComponent<SettingsScreenUi>();

        std::vector<std::string> names;
        for (const auto& category : SettingsCatalog())
            names.push_back(category.name);
        view_->BuildTabs(names);
        view_->PlayEnter();
        Refresh();

        // 開いた決定ボタンを押したままでも、値を切り替えない
        previousKeys_ = ReadKeys();
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);
    }

    void SettingsScreenPresenter::OnUpdate()
    {
        if (isClosed_ || !view_)
            return;

        const Keys keys = ReadKeys();
        const Keys pressed{
            keys.up      && !previousKeys_.up,
            keys.down    && !previousKeys_.down,
            keys.left    && !previousKeys_.left,
            keys.right   && !previousKeys_.right,
            keys.prevTab && !previousKeys_.prevTab,
            keys.nextTab && !previousKeys_.nextTab,
            keys.confirm && !previousKeys_.confirm,
            keys.cancel  && !previousKeys_.cancel,
        };
        previousKeys_ = keys;

        if (pressed.cancel)
            Close();
        else if (pressed.up)
            MoveRow(-1);
        else if (pressed.down)
            MoveRow(1);
        else if (pressed.left)
            ChangeValue(-1);
        else if (pressed.right || pressed.confirm)
            ChangeValue(1);
        else if (pressed.prevTab)
            ChangeCategory(-1);
        else if (pressed.nextTab)
            ChangeCategory(1);
    }

    void SettingsScreenPresenter::MoveRow(const int delta)
    {
        const auto& items = SettingsCatalog()[category_].items;
        if (items.empty())
            return;

        const int next = std::clamp(static_cast<int>(selectedRow_) + delta, 0, static_cast<int>(items.size()) - 1);
        if (static_cast<size_t>(next) == selectedRow_)
            return;

        selectedRow_ = static_cast<size_t>(next);
        const size_t visible = std::max<size_t>(view_->VisibleRowCount(), 1);
        if (selectedRow_ < firstVisible_)
            firstVisible_ = selectedRow_;
        else if (selectedRow_ >= firstVisible_ + visible)
            firstVisible_ = selectedRow_ + 1 - visible;

        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        Refresh();
    }

    void SettingsScreenPresenter::ChangeValue(const int delta)
    {
        const auto& items = SettingsCatalog()[category_].items;
        if (selectedRow_ >= items.size())
            return;

        const auto& item = items[selectedRow_];
        const int count = static_cast<int>(item.choices.size());
        if (count <= 1)
            return;

        // 端で止めず、ぐるりと回す (2択なら押すたびに入れ替わる)。wrap が無い行は端で止める
        const int current = item.get();
        const int next    = item.wrap ? (current + delta + count) % count : std::clamp(current + delta, 0, count - 1);
        if (next == current)
            return;

        item.set(next);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        Refresh();
    }

    void SettingsScreenPresenter::ChangeCategory(const int delta)
    {
        const int count = static_cast<int>(SettingsCatalog().size());
        if (count <= 1)
            return;

        category_     = static_cast<size_t>((static_cast<int>(category_) + delta + count) % count);
        selectedRow_  = 0;
        firstVisible_ = 0;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Cursor);
        Refresh();
    }

    void SettingsScreenPresenter::Refresh()
    {
        const auto& category = SettingsCatalog()[category_];
        view_->SetTabSelection(category_);
        view_->SetCategoryName(category.name);

        const size_t visible = view_->VisibleRowCount();
        for (size_t slot = 0; slot < visible; ++slot)
        {
            const size_t index = firstVisible_ + slot;
            if (index >= category.items.size())
            {
                view_->SetRow(slot, "", "", false);
                continue;
            }

            const auto& item = category.items[index];
            const int value = std::clamp(item.get(), 0, static_cast<int>(item.choices.size()) - 1);
            view_->SetRow(slot, item.label, item.choices[value], index == selectedRow_);
        }
        view_->SetScroll(firstVisible_, category.items.size());

        std::string description;
        if (selectedRow_ < category.items.size())
        {
            const auto& item = category.items[selectedRow_];
            const size_t value = static_cast<size_t>(std::max(item.get(), 0));
            if (value < item.descriptions.size())
                description = item.descriptions[value];
        }
        view_->SetDescription(description);
    }

    void SettingsScreenPresenter::Close()
    {
        if (isClosed_)
            return;
        isClosed_ = true;

        if (isOwner_)
            isOpen_ = false;
        isOwner_ = false;
        GameCore::GameSettings::GetInstance().Save();
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);
        Entity().lock()->OnDestroy();
    }

    void SettingsScreenPresenter::OnDestroy()
    {
        if (isOwner_)
            isOpen_ = false;
        isOwner_ = false;
    }

    SettingsScreenPresenter::Keys SettingsScreenPresenter::ReadKeys()
    {
        const auto xInput = Gamepad::Get();
        return Keys{
            .up      = Keyboard::IsDown(Key::Up) || Keyboard::IsDown(Key::W)
                       || xInput.IsDown(GamepadButton::DPadUp) || xInput.thumbLY > SETTINGS_STICK_DEADZONE,
            .down    = Keyboard::IsDown(Key::Down) || Keyboard::IsDown(Key::S)
                       || xInput.IsDown(GamepadButton::DPadDown) || xInput.thumbLY < -SETTINGS_STICK_DEADZONE,
            .left    = Keyboard::IsDown(Key::Left) || Keyboard::IsDown(Key::A)
                       || xInput.IsDown(GamepadButton::DPadLeft) || xInput.thumbLX < -SETTINGS_STICK_DEADZONE,
            .right   = Keyboard::IsDown(Key::Right) || Keyboard::IsDown(Key::D)
                       || xInput.IsDown(GamepadButton::DPadRight) || xInput.thumbLX > SETTINGS_STICK_DEADZONE,
            .prevTab = Keyboard::IsDown(Key::Q) || xInput.IsDown(GamepadButton::LeftShoulder),
            .nextTab = Keyboard::IsDown(Key::E) || xInput.IsDown(GamepadButton::RightShoulder),
            .confirm = Keyboard::IsDown(Key::Return) || Keyboard::IsDown(Key::Space) || xInput.IsDown(GamepadButton::A),
            .cancel  = Keyboard::IsDown(Key::Escape) || Keyboard::IsDown(Key::Back)
                       || xInput.IsDown(GamepadButton::B) || xInput.IsDown(GamepadButton::Start),
        };
    }

    void SettingsScreenPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        ImGui::Text("category: %d  row: %d  first: %d", static_cast<int>(category_),
                    static_cast<int>(selectedRow_), static_cast<int>(firstVisible_));
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SettingsScreenPresenter);
#pragma endregion
