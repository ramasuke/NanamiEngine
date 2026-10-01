#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <Windows.h>

struct ImFont;

class NANAMI_API ImGuiWrapper
{
public:
    static constexpr float LARGE_FONT_SIZE = 26.0f;

    static void CreateInstance();

    static ImGuiWrapper& Instance();

    void Init();
    void Update();
    void Draw();
    void OnDestroy();

    [[nodiscard]] ImFont* LargeFont() const { return largeFont_; }

private:
    static ImGuiWrapper* instance_;

    ImFont* largeFont_ = nullptr;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ImGuiWrapper();
    ImGuiWrapper(const ImGuiWrapper&);
    ~ImGuiWrapper();

    void UpdateInputMouse();
    void UpdateNewFrame();
};