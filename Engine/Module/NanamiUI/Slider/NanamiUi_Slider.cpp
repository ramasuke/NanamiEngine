#include "NanamiUi_Slider.h"
#include <algorithm>

#include "DxLib.h"
#include "../../GameObject/Transform/Transform.h"

namespace NanamiEngine::Module::NanamiUi
{
    void Slider::SetValue(const float value)
    {
        value_ = std::clamp(value, 0.0f, 1.0f);
    }

    void Slider::OnUserInterfaceRender()
    {
        if (!IsEnable() || !gaugeSprite_)
            return;

        const auto worldPos = Transform().GetWorldPos();
        const int w = static_cast<int>(drawSize_.x);
        const int h = static_cast<int>(drawSize_.y);
        const int maskedScreen = MakeScreen(w, h, true);

        // ---------- マスク生成フェーズ ----------
        SetDrawScreen(maskedScreen);
        ClsDrawScreen();

        const auto renderRot   = Transform().GetWorldRot  ();
        const auto renderScale = Transform().GetWorldScale();

        const float angle = renderRot  .z;
        const float scale = renderScale.x;

        // ★ マスク内ローカル座標で描画
        DrawRotaGraphF(
            drawPosition_.x,
            drawPosition_.y,
            scale,
            angle,
            gaugeSprite_->GetDxLibHandle(),
            TRUE
        );

        // 減少分を黒で塗る
        const int lostWidth = static_cast<int>(w * (1.0f - value_));
        DrawBox(w - lostWidth, 0, w, h, GetColor(0, 0, 0), TRUE);

        // 黒を透過
        GraphFilter(maskedScreen, DX_GRAPH_FILTER_BRIGHT_CLIP, DX_CMP_LESS, 20, TRUE, GetColor(0, 255, 0), 0);
        SetDrawScreen(DX_SCREEN_BACK);

        // ---------- 最終描画フェーズ ----------
        const int x = static_cast<int>(worldPos.x);
        const int y = static_cast<int>(worldPos.y);

        GraphFilter(maskedScreen, DX_GRAPH_FILTER_BRIGHT_CLIP, DX_CMP_GREATER, 128, TRUE, GetColor(0, 255, 0), 0);
        DrawGraph(x, y, maskedScreen, TRUE);
        DeleteGraph(maskedScreen);

    }
    
    void Slider::OnDrawGui()
    {
        ImGui::Text("Slider");

        ImGui::SliderFloat("value_", &value_, 0.0f, 1.0f);

        float position[2] = { drawPosition_.x,  drawPosition_.y };
        float size    [2] = { drawSize_    .x, drawSize_     .y };

        if (ImGui::InputFloat2("drawPos_", position))
        {
            drawPosition_.x = position[0];
            drawPosition_.y = position[1];
        }

        if (ImGui::InputFloat2("drawSize_", size))
        {
            drawSize_.x = size[0];
            drawSize_.y = size[1];
        }

        ImGuiHelper::OnDrawInputField("gaugeSprite_", gaugeSprite_);
        value_ = std::clamp(value_, 0.0f, 1.0f);
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);

        const int w = static_cast<int>(drawSize_.x);
        const int h = static_cast<int>(drawSize_.y);
        const auto worldPos = Transform().GetWorldPos();
        const int x = static_cast<int>(worldPos.x);
        const int y = static_cast<int>(worldPos.y);
        DrawBox(x, y, x + w, y + h, GetColor(255, 255, 255), FALSE);
    }
}
