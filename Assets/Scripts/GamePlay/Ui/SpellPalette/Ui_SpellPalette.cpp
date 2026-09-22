#include "Ui_SpellPalette.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "trigonometric.hpp"
#include "Ui_SpellSlot.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/Magic/IMagicSpell.h"
#include "../../../Core/Game/PlayerAvatar/Input/PlayerAvatarInput_void.h"
#include "../../PlayerAvatar/MagicCaster/MagicCasterAvatar.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarInputDevice;
        using GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStateType;
        using GameCore::PlayerAvatar::MagicCaster::SPELL_LOADOUT_SLOT_COUNT;
        using GameCore::PlayerAvatar::MagicCaster::SPELL_SLOTS_PER_PAGE;

        /** 向き（0=上 1=右 2=下 3=左）の外向きの単位ベクトル。ボタン表示を枠の外側の頂点に置くのに使う */
        const std::array<glm::vec2, SPELL_SLOTS_PER_PAGE> SPELL_PALETTE_DIRECTIONS =
        {
            glm::vec2( 0.0f, -1.0f),
            glm::vec2( 1.0f,  0.0f),
            glm::vec2( 0.0f,  1.0f),
            glm::vec2(-1.0f,  0.0f),
        };

        float SpellPaletteMoveTowards(const float current, const float target, const float maxDelta)
        {
            if (current < target)
                return (std::min)(current + maxDelta, target);
            return (std::max)(current - maxDelta, target);
        }

        float SpellPaletteStepRate(const float deltaTime, const float duration_secs)
        {
            return duration_secs > 0.0f ? deltaTime / duration_secs : 1.0f;
        }

        int SpellPaletteToBlendRate(const float alpha)
        {
            return std::clamp(static_cast<int>(alpha), 0, 255);
        }

        std::string SpellPaletteFormatSeconds(const float seconds)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%.1f", seconds);
            return buffer;
        }
    }

    void SpellPalette::Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& avatar)
    {
        avatar_ = avatar;
        SpawnSlots();
    }

    void SpellPalette::SpawnSlots()
    {
        if (isSpawned_ || !slotPrefab_ || !slotsRoot_)
            return;
        isSpawned_ = true;

        const auto slotsObject = slotsRoot_.get();
        for (int i = 0; i < SPELL_LOADOUT_SLOT_COUNT; ++i)
        {
            const auto slotObject = Scene::GameObject::Instantiate(*slotPrefab_.get(), slotsObject).lock();
            slotViews_[static_cast<size_t>(i)] = slotObject ? slotObject->Components().Catch<SpellSlot>() : std::weak_ptr<SpellSlot>{};
        }
    }

    glm::vec3 SpellPalette::FrontAnchorPos(const int direction) const
    {
        const FIELD(GameObject::IGameObject)* anchors[] = { &anchorTop_, &anchorRight_, &anchorBottom_, &anchorLeft_ };
        const auto anchor = anchors[direction]->get();
        return anchor ? anchor->Transform().GetLocalPos() : glm::vec3(0.0f);
    }

    glm::vec3 SpellPalette::BackAnchorPos(const int direction) const
    {
        // 奥のページは時計回りに 45° ずらした斜めに置く。入れ替えると陣が回ったように見える
        const FIELD(GameObject::IGameObject)* anchors[] = { &anchorTopRight_, &anchorBottomRight_, &anchorBottomLeft_, &anchorTopLeft_ };
        const auto anchor = anchors[direction]->get();
        return anchor ? anchor->Transform().GetLocalPos() : glm::vec3(0.0f);
    }

    void SpellPalette::RefreshSpells()
    {
        const auto avatar = avatar_.lock();
        const auto resources = avatar ? avatar->Resources().lock() : nullptr;
        if (!resources)
            return;

        for (int i = 0; i < SPELL_LOADOUT_SLOT_COUNT; ++i)
        {
            const auto view = slotViews_[static_cast<size_t>(i)].lock();
            if (!view)
                continue;

            const auto spell = resources->LoadoutSpell(i);
            view->SetSpell(spell ? spell->IconSprite() : nullptr,
                           spell ? std::to_string(static_cast<int>(spell->ManaCost())) : std::string());
            view->SetManaLack(false);
            shownManaLack_[static_cast<size_t>(i)] = false;
        }
    }

    void SpellPalette::ApplyDeviceGlyphs()
    {
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;
        const FIELD(Asset::SpriteFile)* padGlyphs[] = { &padGlyphTop_, &padGlyphRight_, &padGlyphBottom_, &padGlyphLeft_ };
        const FIELD(Asset::SpriteFile)* keyGlyphs[] = { &keyGlyphTop_, &keyGlyphRight_, &keyGlyphBottom_, &keyGlyphLeft_ };

        for (int i = 0; i < SPELL_LOADOUT_SLOT_COUNT; ++i)
        {
            const auto view = slotViews_[static_cast<size_t>(i)].lock();
            if (!view)
                continue;

            const int direction = i % SPELL_SLOTS_PER_PAGE;
            const auto& glyph = isPad ? *padGlyphs[direction] : *keyGlyphs[direction];
            view->SetGlyph(glyph.get(), SPELL_PALETTE_DIRECTIONS[static_cast<size_t>(direction)]);
        }

        if (paletteGlyph_) paletteGlyph_->SetSprite((isPad ? padPaletteSprite_ : keyPaletteSprite_).get());
        if (pageGlyph_)    pageGlyph_   ->SetSprite((isPad ? padPageSprite_    : keyPageSprite_   ).get());
    }

    void SpellPalette::PresentSlots(const float groupAlpha)
    {
        const auto avatar = avatar_.lock();
        const auto resources = avatar ? avatar->Resources().lock() : nullptr;
        if (!avatar || !resources)
            return;

        const auto& status = avatar->PlayerStatus();
        const float mana = status.Mana().CurrentValue().Value();

        for (int i = 0; i < SPELL_LOADOUT_SLOT_COUNT; ++i)
        {
            const auto view = slotViews_[static_cast<size_t>(i)].lock();
            if (!view)
                continue;

            const int page      = i / SPELL_SLOTS_PER_PAGE;
            const int direction = i % SPELL_SLOTS_PER_PAGE;
            const float frontWeight = page == 0 ? 1.0f - pageBlend_ : pageBlend_;

            if (const auto viewObject = view->Entity().lock())
            {
                const glm::vec3 pos = BackAnchorPos(direction) + (FrontAnchorPos(direction) - BackAnchorPos(direction)) * frontWeight;
                viewObject->Transform().SetLocalPos(pos);
            }

            const auto spell = resources->LoadoutSpell(i);
            const bool isLacking = spell && mana < spell->ManaCost();
            if (isLacking != shownManaLack_[static_cast<size_t>(i)])
            {
                shownManaLack_[static_cast<size_t>(i)] = isLacking;
                view->SetManaLack(isLacking);
            }

            const float cooldownRate = status.CooldownRemainingRate(i);
            if (cooldownRate > 0.0f)
                view->SetCooldownText(SpellPaletteFormatSeconds(status.CooldownRemaining_secs(i)));

            const float bodyAlpha = groupAlpha * (backAlphaRate_ + (1.0f - backAlphaRate_) * frontWeight);
            const float frontAlpha = bodyAlpha * frontWeight;
            view->Apply(SpellSlot::Appearance{
                .scale         = backScale_ + (1.0f - backScale_) * frontWeight,
                .bodyAlpha     = SpellPaletteToBlendRate(bodyAlpha),
                .iconAlpha     = SpellPaletteToBlendRate(spell ? bodyAlpha * (isLacking ? manaLackIconRate_ : 1.0f) : 0.0f),
                .activeAlpha   = SpellPaletteToBlendRate(frontAlpha * openRate_),
                .manaLackAlpha = SpellPaletteToBlendRate(isLacking ? bodyAlpha : 0.0f),
                .cooldownRate  = cooldownRate,
                .cooldownAlpha = SpellPaletteToBlendRate(cooldownRate > 0.0f ? bodyAlpha : 0.0f),
                .textAlpha     = SpellPaletteToBlendRate(spell ? frontAlpha : 0.0f),
                .glyphAlpha    = SpellPaletteToBlendRate(frontAlpha),
            });
        }
    }

    void SpellPalette::PresentNames(const float groupAlpha) const
    {
        const auto avatar = avatar_.lock();
        const auto resources = avatar ? avatar->Resources().lock() : nullptr;
        const FIELD(NanamiUi::TextRenderer)* names[] = { &nameTop_, &nameRight_, &nameBottom_, &nameLeft_ };
        const int nameAlpha = SpellPaletteToBlendRate(groupAlpha * openRate_);

        for (int direction = 0; direction < SPELL_SLOTS_PER_PAGE; ++direction)
        {
            const auto name = names[direction]->get();
            if (!name)
                continue;

            const auto spell = resources ? resources->LoadoutSpell(FrontPage() * SPELL_SLOTS_PER_PAGE + direction) : nullptr;
            name->SetText(spell ? spell->DisplayName() : std::string());
            name->SetBlendRate(nameAlpha);
        }
    }

    void SpellPalette::PresentMana(const float groupAlpha) const
    {
        const auto avatar = avatar_.lock();
        if (!avatar)
            return;

        const auto& status = avatar->PlayerStatus();
        const float mana = status.Mana().CurrentValue().Value();
        const float maxMana = status.MaxMana().Value();
        const float rate = maxMana > 0.0f ? std::clamp(mana / maxMana, 0.0f, 1.0f) : 0.0f;
        const int alpha = SpellPaletteToBlendRate(groupAlpha);

        if (manaFill_)
        {
            manaFill_->SetFillRate(rate);
            manaFill_->SetBlendRate(alpha);
        }
        if (manaTip_)
        {
            const float angle = glm::radians(manaArcStartDeg_ + manaArcSpanDeg_ * rate);
            const glm::vec3 tipPos = manaTip_->Transform().GetLocalPos();
            manaTip_->Transform().SetLocalPos(glm::vec3(std::sin(angle) * manaArcRadius_, -std::cos(angle) * manaArcRadius_, tipPos.z));
            manaTip_->SetBlendRate(rate > 0.01f ? alpha : 0);
        }
        if (manaValue_)
        {
            manaValue_->SetText(std::to_string(static_cast<int>(mana)));
            manaValue_->SetBlendRate(alpha);
        }
        if (manaLabel_) manaLabel_->SetBlendRate(alpha);
        if (manaTrack_) manaTrack_->SetBlendRate(alpha);
    }

    void SpellPalette::FadeOut()
    {
        for (const auto& weakView : slotViews_)
        {
            if (const auto view = weakView.lock())
                view->Apply(SpellSlot::Appearance{});
        }
        PresentNames(0.0f);
        PresentMana(0.0f);
        if (halo_)         halo_        ->SetBlendRate(0);
        if (circle_)       circle_      ->SetBlendRate(0);
        if (paletteGlyph_) paletteGlyph_->SetBlendRate(0);
        if (pageGlyph_)    pageGlyph_   ->SetBlendRate(0);
        if (pageText_)     pageText_    ->SetBlendRate(0);
    }

    void SpellPalette::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        const auto avatar = avatar_.lock();
        if (!avatar)
        {
            visibleRate_ = 0.0f;
            FadeOut();
            return;
        }

        const auto& input = avatar->GetInputAction();
        const auto device = input.CurrentDevice();
        if (isDeviceDirty_ || device != device_)
        {
            device_ = device;
            isDeviceDirty_ = false;
            ApplyDeviceGlyphs();
        }
        if (isSpellsDirty_)
        {
            isSpellsDirty_ = false;
            RefreshSpells();
        }

        const auto state = avatar->GetStateMachine().GetCurrentStateType();
        const bool isShown = state != MagicCasterAvatarStateType::Disable
                          && state != MagicCasterAvatarStateType::Chatting
                          && state != MagicCasterAvatarStateType::Death;
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;
        // キーボードは 1〜4 を直接押すので、右クリック（2ページ目）を押している間だけ開いた見た目にする
        const bool isOpen = isShown && (input.Palette().IsUpdatePressed() || (!isPad && input.PageShift().IsUpdatePressed()));

        const float fadeStep = SpellPaletteStepRate(deltaTime, fadeDuration_secs_);
        visibleRate_ = SpellPaletteMoveTowards(visibleRate_, isShown ? 1.0f : 0.0f, fadeStep);
        openRate_    = SpellPaletteMoveTowards(openRate_, isOpen ? 1.0f : 0.0f, fadeStep);
        pageBlend_   = SpellPaletteMoveTowards(pageBlend_, input.IsSecondPage() ? 1.0f : 0.0f,
                                               SpellPaletteStepRate(deltaTime, pageSwapDuration_secs_));

        const float groupAlpha = 255.0f * visibleRate_ * (idleAlphaRate_ + (1.0f - idleAlphaRate_) * openRate_);
        const int groupBlendRate = SpellPaletteToBlendRate(groupAlpha);

        if (halo_)         halo_        ->SetBlendRate(SpellPaletteToBlendRate(groupAlpha * openRate_));
        if (circle_)       circle_      ->SetBlendRate(groupBlendRate);
        if (paletteGlyph_) paletteGlyph_->SetBlendRate(groupBlendRate);
        if (pageGlyph_)    pageGlyph_   ->SetBlendRate(groupBlendRate);
        if (pageText_)
        {
            pageText_->SetText(FrontPage() == 0 ? "Ⅱ" : "Ⅰ");
            pageText_->SetBlendRate(groupBlendRate);
        }

        PresentSlots(groupAlpha);
        PresentNames(groupAlpha);
        PresentMana(groupAlpha);
        if (manaValue_)
            manaValue_->SetTextColor(manaValueColor_);
    }

    void SpellPalette::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("slotsRoot_", slotsRoot_);
        ImGuiHelper::OnDrawInputField("slotPrefab_", slotPrefab_);
        ImGuiHelper::OnDrawInputField("anchorTop_", anchorTop_);
        ImGuiHelper::OnDrawInputField("anchorRight_", anchorRight_);
        ImGuiHelper::OnDrawInputField("anchorBottom_", anchorBottom_);
        ImGuiHelper::OnDrawInputField("anchorLeft_", anchorLeft_);
        ImGuiHelper::OnDrawInputField("anchorTopRight_", anchorTopRight_);
        ImGuiHelper::OnDrawInputField("anchorBottomRight_", anchorBottomRight_);
        ImGuiHelper::OnDrawInputField("anchorBottomLeft_", anchorBottomLeft_);
        ImGuiHelper::OnDrawInputField("anchorTopLeft_", anchorTopLeft_);
        ImGuiHelper::OnDrawInputField("nameTop_", nameTop_);
        ImGuiHelper::OnDrawInputField("nameRight_", nameRight_);
        ImGuiHelper::OnDrawInputField("nameBottom_", nameBottom_);
        ImGuiHelper::OnDrawInputField("nameLeft_", nameLeft_);
        ImGuiHelper::OnDrawInputField("halo_", halo_);
        ImGuiHelper::OnDrawInputField("circle_", circle_);
        ImGuiHelper::OnDrawInputField("manaTrack_", manaTrack_);
        ImGuiHelper::OnDrawInputField("manaFill_", manaFill_);
        ImGuiHelper::OnDrawInputField("manaTip_", manaTip_);
        ImGuiHelper::OnDrawInputField("manaLabel_", manaLabel_);
        ImGuiHelper::OnDrawInputField("manaValue_", manaValue_);
        ImGuiHelper::OnDrawInputField("paletteGlyph_", paletteGlyph_);
        ImGuiHelper::OnDrawInputField("pageGlyph_", pageGlyph_);
        ImGuiHelper::OnDrawInputField("pageText_", pageText_);
        ImGuiHelper::OnDrawInputField("padGlyphTop_", padGlyphTop_);
        ImGuiHelper::OnDrawInputField("padGlyphRight_", padGlyphRight_);
        ImGuiHelper::OnDrawInputField("padGlyphBottom_", padGlyphBottom_);
        ImGuiHelper::OnDrawInputField("padGlyphLeft_", padGlyphLeft_);
        ImGuiHelper::OnDrawInputField("keyGlyphTop_", keyGlyphTop_);
        ImGuiHelper::OnDrawInputField("keyGlyphRight_", keyGlyphRight_);
        ImGuiHelper::OnDrawInputField("keyGlyphBottom_", keyGlyphBottom_);
        ImGuiHelper::OnDrawInputField("keyGlyphLeft_", keyGlyphLeft_);
        ImGuiHelper::OnDrawInputField("padPaletteSprite_", padPaletteSprite_);
        ImGuiHelper::OnDrawInputField("keyPaletteSprite_", keyPaletteSprite_);
        ImGuiHelper::OnDrawInputField("padPageSprite_", padPageSprite_);
        ImGuiHelper::OnDrawInputField("keyPageSprite_", keyPageSprite_);
        ImGuiHelper::OnDrawInputField("manaArcRadius_", manaArcRadius_);
        ImGuiHelper::OnDrawInputField("manaArcStartDeg_", manaArcStartDeg_);
        ImGuiHelper::OnDrawInputField("manaArcSpanDeg_", manaArcSpanDeg_);
        ImGuiHelper::OnDrawInputField("idleAlphaRate_", idleAlphaRate_);
        ImGuiHelper::OnDrawInputField("backScale_", backScale_);
        ImGuiHelper::OnDrawInputField("backAlphaRate_", backAlphaRate_);
        ImGuiHelper::OnDrawInputField("manaLackIconRate_", manaLackIconRate_);
        ImGuiHelper::OnDrawInputField("fadeDuration_secs_", fadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("pageSwapDuration_secs_", pageSwapDuration_secs_);
        manaValueColor_.OnDrawGui();
        if (ImGui::Button("Refresh Spells"))
            isSpellsDirty_ = true;
    }
}
