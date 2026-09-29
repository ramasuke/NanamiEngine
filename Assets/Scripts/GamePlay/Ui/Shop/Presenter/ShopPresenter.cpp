#include "ShopPresenter.h"

#include <algorithm>


#include "../../../Prop/MerchantStall/Prop_MerchantStall.h"
#include "../../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "../../../../Core/Game/PlayerAvatar/Wallet/PlayerAvatar_Wallet.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void ShopPresenter::Bind(const std::weak_ptr<Prop::MerchantStall>& stall)
    {
        stall_ = stall;
    }

    void ShopPresenter::OnStart()
    {
        screen_ = RequireComponent<UiFlow::UiScreen>();
        if (!screen_->Open())
        {
            Entity().lock()->OnDestroy();
            return;
        }
        screen_->Input().SetRepeat(quantityRepeatDelay_secs_, quantityRepeatInterval_secs_);
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Open);

        view_ = RequireComponent<ShopUi>();

        const auto owner = GameCore::PlayerAvatar::Owner();
        suspendedAvatar_ = owner;
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

        // Select は同じ index だと通知を出さないので、初期表示はここで一度だけ作る
        Refresh();
    }

    void ShopPresenter::OnUpdate()
    {
        if (!view_ || !model_ || !screen_->IsFocused())
            return;

        using UiFlow::UiAction;
        auto& input = screen_->Input();

        if (input.IsPressed(UiAction::Up))
            model_->Cursor().Move(-1);
        if (input.IsPressed(UiAction::Down))
            model_->Cursor().Move(1);

        // 左右を同時に押している間は増減しない
        if (input.IsHeld(UiAction::Left) != input.IsHeld(UiAction::Right))
        {
            if (input.IsRepeated(UiAction::Left))
                ChangeQuantity(-1);
            if (input.IsRepeated(UiAction::Right))
                ChangeQuantity(1);
        }

        if (input.IsPressed(UiAction::Submit))
            Purchase();
        if (input.IsPressed(UiAction::Cancel))
            Close();
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
        if (!screen_->IsOpen())
            return;
        Sound::UiSoundBank::Play(uiSounds_, Sound::UiSe::Close);

        if (const auto stall = stall_.lock())
            stall->RestoreCamera();
        screen_->Close();
    }

    void ShopPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shop_", shop_);
        ImGuiHelper::OnDrawInputField("purchaseSound_", purchaseSound_);
        ImGuiHelper::OnDrawInputField("refuseSound_", refuseSound_);
        ImGuiHelper::OnDrawInputField("cursorSound_", cursorSound_);
        ImGuiHelper::OnDrawInputField("quantityRepeatDelay_secs_", quantityRepeatDelay_secs_);
        ImGuiHelper::OnDrawInputField("quantityRepeatInterval_secs_", quantityRepeatInterval_secs_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopPresenter);
#pragma endregion
