#include <DxLib.h>
#include "../ImGui/backends/imgui_impl_dx11.h"
#include "../ImGui/backends/imgui_impl_win32.h"
#include "ImGuiWrapper.h"

ImGuiWrapper* ImGuiWrapper::instance_ = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ImGuiWrapper::CreateInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new ImGuiWrapper();
	}
	instance_->Init();
}

ImGuiWrapper& ImGuiWrapper::Instance()
{
	return *instance_;
}

void ImGuiWrapper::Init()
{
	SetHookWinProc(WndProc);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	const ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/msgothic.ttc",
		13.0f,
		nullptr,
		io.Fonts->GetGlyphRangesJapanese()
	);

	ImGui_ImplWin32_Init(GetMainWindowHandle());
	ImGui_ImplDX11_Init(
		(ID3D11Device*)GetUseDirect3D11Device(),
		(ID3D11DeviceContext*)GetUseDirect3D11DeviceContext()
	);
}

void ImGuiWrapper::Update()
{
	UpdateInputMouse();
	UpdateNewFrame();
}

void ImGuiWrapper::Draw()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	RefreshDxLibDirect3DSetting();
}

void ImGuiWrapper::OnDestroy()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete instance_;
}


LRESULT ImGuiWrapper::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		SetUseHookWinProcReturnValue(TRUE);
		return 0;
	}

	switch (msg)
	{
	case WM_IME_SETCONTEXT:
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_NOTIFY:
	case WM_IME_REQUEST:
		SetUseHookWinProcReturnValue(TRUE);
		return DefWindowProc(hWnd, msg, wParam, lParam);

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) {
			SetUseHookWinProcReturnValue(TRUE);
			return 0;
		}
		break;
	}

	SetUseHookWinProcReturnValue(FALSE);
	return 0;
}

ImGuiWrapper::~ImGuiWrapper()
{
}

void ImGuiWrapper::UpdateInputMouse()
{
	ImGuiIO& io = ImGui::GetIO();
	const auto mouseInput = GetMouseInput();
	int mousePosX = 0;
	int mousePosY = 0;
	GetMousePoint(&mousePosX, &mousePosY);
	io.AddMousePosEvent(mousePosX, mousePosY);
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, mouseInput & MOUSE_INPUT_LEFT);
	io.AddMouseButtonEvent(ImGuiMouseButton_Right, mouseInput & MOUSE_INPUT_RIGHT);

}

void ImGuiWrapper::UpdateNewFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

ImGuiWrapper::ImGuiWrapper()
{
}

ImGuiWrapper::ImGuiWrapper(const ImGuiWrapper&)
{
}
