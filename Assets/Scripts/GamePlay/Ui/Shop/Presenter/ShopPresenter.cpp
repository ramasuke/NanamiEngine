#include "ShopPresenter.h"

#include <algorithm>

#include "DxLib.h"

#include "../../../Prop/MerchantStall/Prop_MerchantStall.h"
#include "../../../Sound/UiSoundBank.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 左スティックを方向キーとして読むためのしきい値
        constexpr short SHOP_STICK_DEADZONE = 12000;
    }

    bool ShopPresenter::IsAnotherOpen() const
    {
        bool found = false;
        NanamiEngine::Core::Application::ApplicationBase::GameWindow()->MainScene().ForEachGameObject(
            [this, &found](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                if (found)
                    return;

                const auto presenter = gameObject->Components().Catch<ShopPresenter>().lock();
                found = presenter && presenter.get() != this && presenter->isOpen_;
            });
        return found;
    }

    void ShopPresenter::Bind(const std::weak_ptr<Prop::MerchantStall>& stall)
    {
        stall_ = stall;
    }

    void ShopPresenter::OnStart()
    {
        if (IsAnotherOpen())
        {
            isClosed_ = true;
            Entity().lock()->OnDestroy();
            return;
        }
        isOpen_ = true;
        Sound::UiSoundBank::Play(Sound::UiSe::Open);

        view_ = RequireComponent<ShopUi>();

        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;
        if (owner)
            owner->DisableStateMachine();
        if (const auto stall = stall_.lock())
            stall->FocusCamera();

        std::vector<ShopItemEntry> entries;
        const auto shop = shop_.get();
        if (shop)
        {
            for (const auto& entry : shop->Entries())
            {
                if (const auto item = entry.Item())
                    entries.push_back(ShopItemEntry{ item, entry.Price() });
            }
            view_->SetTitle(shop->Title());
        }

        auto* status = owner ? &owner->PlayerStatus() : nullptr;
        model_ = std::make_unique<ShopModel>(
            std::move(entries),
            view_->MaxVisibleRows(),
            status ? &status->Wallet() : nullptr,
            status ? &status->Pouch() : nullptr);

        view_->BuildRows(std::min(model_->Entries().size(), model_->Cursor().VisibleRowCount()));
        view_->SubscribeOnClickRow([this](const size_t row)
        {
            model_->Cursor().Select(model_->Cursor().FirstVisibleIndex() + row);
        });
        model_->Cursor().OnSelectionChanged().Subscribe([this](size_t)
        {
            model_->ResetQuantity();
            PlaySound(cursorSound_);
            Refresh();
        }).AddTo(this);

        if (status)
        {
            status->Wallet().Observe().Subscribe([this](const GameCore::StatusParameter::Money balance)
            {
                view_->SetMoney(balance.Value());
            }).AddTo(this);
        }

        // 話しかけたときの押しっぱなしを、開いた直後の入力として拾わない
        previousKeys_ = ReadKeys();

        // Select は同じ index だと通知を出さないので、初期表示はここで一度だけ作る
        Refresh();
    }

    void ShopPresenter::OnUpdate()
    {
        if (isClosed_)
            return;

        // 閉じたフレームに有効へ戻すと、閉じた B をジャンプとして拾ってしまう
        if (isClosing_)
        {
            isClosed_ = true;
            if (const auto owner = suspendedAvatar_.lock())
                owner->EnableStateMachiine();
            Entity().lock()->OnDestroy();
            return;
        }

        if (!view_ || !model_)
            return;

        const Keys keys = ReadKeys();

        if (keys.prev && !previousKeys_.prev)
            model_->Cursor().Move(-1);
        if (keys.next && !previousKeys_.next)
            model_->Cursor().Move(1);
        UpdateQuantity(keys);
        if (keys.confirm && !previousKeys_.confirm)
            Purchase();

        const bool isCancelPressed = keys.cancel && !previousKeys_.cancel;
        previousKeys_ = keys;
        if (isCancelPressed)
            Close();
    }

    ShopPresenter::Keys ShopPresenter::ReadKeys()
    {
        XINPUT_STATE xInput{};
        GetJoypadXInputState(DX_INPUT_PAD1, &xInput);

        return Keys{
            .prev    = CheckHitKey(KEY_INPUT_UP) || CheckHitKey(KEY_INPUT_W)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_UP] || xInput.ThumbLY > SHOP_STICK_DEADZONE,
            .next    = CheckHitKey(KEY_INPUT_DOWN) || CheckHitKey(KEY_INPUT_S)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_DOWN] || xInput.ThumbLY < -SHOP_STICK_DEADZONE,
            .less    = CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_A)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_LEFT] || xInput.ThumbLX < -SHOP_STICK_DEADZONE,
            .more    = CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_D)
                       || xInput.Buttons[XINPUT_BUTTON_DPAD_RIGHT] || xInput.ThumbLX > SHOP_STICK_DEADZONE,
            .confirm = CheckHitKey(KEY_INPUT_RETURN) || xInput.Buttons[XINPUT_BUTTON_A],
            .cancel  = CheckHitKey(KEY_INPUT_ESCAPE) || xInput.Buttons[XINPUT_BUTTON_B],
        };
    }

    void ShopPresenter::UpdateQuantity(const Keys& keys)
    {
        const int direction = keys.more == keys.less ? 0 : (keys.more ? 1 : -1);
        if (direction == 0)
        {
            quantityHoldDirection_ = 0;
            return;
        }

        if (direction != quantityHoldDirection_)
        {
            quantityHoldDirection_ = direction;
            quantityHold_secs_ = 0.0f;
            quantityRepeat_secs_ = 0.0f;
            ChangeQuantity(direction);
            return;
        }

        // 押し続けたら、少し待ってから一定の間隔で増減を繰り返す
        const float deltaTime = Time::DeltaTime();
        quantityHold_secs_ += deltaTime;
        if (quantityHold_secs_ < quantityRepeatDelay_secs_)
            return;

        quantityRepeat_secs_ += deltaTime;
        if (quantityRepeat_secs_ < quantityRepeatInterval_secs_)
            return;

        quantityRepeat_secs_ = 0.0f;
        ChangeQuantity(direction);
    }

    void ShopPresenter::ChangeQuantity(const int delta)
    {
        if (!model_->ChangeQuantity(delta))
            return;

        PlaySound(cursorSound_);
        Refresh();
    }

    void ShopPresenter::Purchase()
    {
        if (model_->Purchase() <= 0)
        {
            PlaySound(refuseSound_);
            return;
        }

        if (const auto owner = suspendedAvatar_.lock())
            owner->SaveStatus();

        PlaySound(purchaseSound_);
        view_->PlayPaidStamp();
        Refresh();
    }

    void ShopPresenter::Refresh() const
    {
        view_->Bind(*model_);
    }

    void ShopPresenter::PlaySound(const FIELD(Asset::SoundFile)& sound) const
    {
        Sound::UiSoundBank::Play(sound.get());
    }

    void ShopPresenter::Close()
    {
        if (isClosing_)
            return;
        isClosing_ = true;
        Sound::UiSoundBank::Play(Sound::UiSe::Close);

        if (const auto stall = stall_.lock())
            stall->RestoreCamera();
    }

    void ShopPresenter::OnDestroy()
    {
        isOpen_ = false;
    }

    void ShopPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shop_", shop_);
        ImGuiHelper::OnDrawInputField("purchaseSound_", purchaseSound_);
        ImGuiHelper::OnDrawInputField("refuseSound_", refuseSound_);
        ImGuiHelper::OnDrawInputField("cursorSound_", cursorSound_);
        ImGuiHelper::OnDrawInputField("quantityRepeatDelay_secs_", quantityRepeatDelay_secs_);
        ImGuiHelper::OnDrawInputField("quantityRepeatInterval_secs_", quantityRepeatInterval_secs_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopPresenter);
#pragma endregion
