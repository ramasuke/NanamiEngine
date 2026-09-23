#include "ScreenColorGradeRenderer.h"

#include <algorithm>
#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::NanamiUi
{
    void ScreenColorGradeRenderer::SetSaturation(const int saturation)
    {
        saturation_ = (std::max)(saturation, -255);
    }

    void ScreenColorGradeRenderer::SetBright(const int bright)
    {
        bright_ = std::clamp(bright, -255, 255);
    }

    void ScreenColorGradeRenderer::OnUserInterfaceRender()
    {
        if (!IsEnable() || (saturation_ == 0 && bright_ == 0))
            return;

        int width  = 0;
        int height = 0;
        GetDrawScreenSize(&width, &height);
        if (screenHandle_ == -1 || width != screenWidth_ || height != screenHeight_)
        {
            if (screenHandle_ != -1)
                DeleteGraph(screenHandle_);

            //NOTE: 非同期読み込みが有効なまま作ると読み込み中のハンドルになり、直後の取り込みで完了待ちに入るので同期で作る
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            screenHandle_ = MakeScreen(width, height, FALSE);
            SetUseASyncLoadFlag(useASyncLoad);
            screenWidth_  = width;
            screenHeight_ = height;
        }
        if (screenHandle_ == -1)
            return;

        GetDrawScreenGraph(0, 0, screenWidth_, screenHeight_, screenHandle_, TRUE);
        GraphFilter(screenHandle_, DX_GRAPH_FILTER_HSB, 0, 0, saturation_, bright_);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
        DrawGraph(0, 0, screenHandle_, FALSE);
    }

    void ScreenColorGradeRenderer::OnDestroy()
    {
        if (screenHandle_ == -1)
            return;

        DeleteGraph(screenHandle_);
        screenHandle_ = -1;
    }

    void ScreenColorGradeRenderer::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("renderOrder_", renderOrder_);
        if (ImGui::SliderInt("saturation_", &saturation_, -255, 0))
            SetSaturation(saturation_);
        if (ImGui::SliderInt("bright_", &bright_, -255, 255))
            SetBright(bright_);
        ImGui::Text("screenHandle_: %d (%dx%d)", screenHandle_, screenWidth_, screenHeight_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::ScreenColorGradeRenderer);
#pragma endregion
