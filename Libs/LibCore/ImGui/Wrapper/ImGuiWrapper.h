#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <Windows.h>

class NANAMI_API ImGuiWrapper
{
public:
    static void CreateInstance();

    static ImGuiWrapper& Instance();

    void Init();
    void Update();
    void Draw();
    void OnDestroy();

private:
    static ImGuiWrapper* instance_;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ImGuiWrapper();
    ImGuiWrapper(const ImGuiWrapper&);
    ~ImGuiWrapper();

    void UpdateInputMouse();
    void UpdateNewFrame();
};