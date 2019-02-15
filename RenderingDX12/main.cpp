#include "CA4G\ca4GScene.h"
#include "ImGui\imgui.h"
#include "ImGui\imgui_impl_win32.h"
#include "ImGui\imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <shlobj.h>

#include "stdafx.h"

#define DX12_ENABLE_DEBUG_LAYER     0


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

template<typename T>
void RenderGUI(gObj<Technique> t) {
	gObj<T> h = t.Dynamic_Cast<T>();
	if (h)
		GuiFor(h);
}

void GuiFor(gObj<IHasBackcolor> t) {
	ImGui::ColorEdit3("clear color", (float*)&t->Backcolor); // Edit 3 floats representing a color
}
void GuiFor(gObj<IHasTriangleNumberParameter> t) {
	ImGui::SliderInt("Number of tris", &t->NumberOfTriangles, t->MinimumOfTriangles, t->MaximumOfTriangles);
}
void GuiFor(gObj<IHasParalellism> t) {
#ifdef MAX_NUMBER_OF_ASYNC_PROCESSES
	ImGui::SliderInt("Number of workers", &t->NumberOfWorkers, 1, MAX_NUMBER_OF_ASYNC_PROCESSES);
#endif
}

LPSTR desktop_directory()
{
	static char path[MAX_PATH + 1];
	if (SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
		return path;
	else
		return "ERROR";
}

int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("CA4G_Samples_Window"), NULL };
    RegisterClassEx(&wc);
    hWnd = CreateWindow(_T("CA4G_Samples_Window"), _T("CA4G Samples"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Show the window
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	static Presenter* presenter = new Presenter(hWnd);
	presenter->Load(technique);
	gObj<IHasBackcolor> asBackcolorRenderer = technique.Dynamic_Cast<IHasBackcolor>();
	gObj<IHasScene> asSceneRenderer = technique.Dynamic_Cast<IHasScene>();
	gObj<IHasCamera> asCameraRenderer = technique.Dynamic_Cast<IHasCamera>();
	gObj<IHasLight> asLightRenderer = technique.Dynamic_Cast<IHasLight>();

	static Scene* scene = nullptr;
	static Camera* camera = new Camera { float3(1,1.5f,2.0f), float3(0,0,0), float3(0,1,0), PI / 4, 0.001f, 1000.0f };
	static LightSource *lightSource = new LightSource{ float3(1,3,2), float3(0,0,0), 0, float3(10, 10, 10) };

	if (asSceneRenderer)
	{
		char * filePath;

		switch (USE_SCENE) {
		case BASIC_OBJ:
			filePath = desktop_directory();
			strcat(filePath, "\\Models\\bunny.obj");
			scene = new Scene(filePath);
			lightSource->Position = float3(2, 2, 0);
			lightSource->Intensity = float3(10, 10, 10);
			break;
		case SPONZA_OBJ:
			filePath = desktop_directory();
			strcat(filePath, "\\Models\\sponza\\SponzaMoreMeshes.obj");
			scene = new Scene(filePath);
			camera->Position = float3(6, 2, 0);
			camera->Target = float3(0, 2, 0);
			lightSource->Position = float3(0, 2, 0);
			lightSource->Intensity = float3(10, 10, 10);
			break;
		case SIBENIK_OBJ:
			filePath = desktop_directory();
			strcat(filePath, "\\Models\\sibenik\\sibenik.obj");
			scene = new Scene(filePath);
			camera->Position = float3(-1, -0.5f, 0);
			camera->Target = float3(0, 0, 0);
			lightSource->Position = float3(0, 2, 0);
			lightSource->Intensity = float3(10, 10, 10);
			break;
		case SANMIGUEL_OBJ:
			filePath = desktop_directory();
			strcat(filePath, "\\Models\\san-miguel\\san-miguel.obj");
			scene = new Scene(filePath);
			camera->Position = float3(-4, 3, 0);
			camera->Target = float3(0, 0, 0);
			lightSource->Position = float3(0, 2, 0);
			lightSource->Intensity = float3(10, 10, 10);
			break;
		}

		asSceneRenderer->Scene = scene;
	}

	if (asCameraRenderer) {
		asCameraRenderer->Camera = camera;
	}

	if (asLightRenderer) {
		asLightRenderer->Light = lightSource;
	}

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(presenter->getInnerD3D12Device(), 3, 
        DXGI_FORMAT_R8G8B8A8_UNORM,
        presenter->getInnerDescriptorsHeap()->GetCPUDescriptorHandleForHeapStart(), 
		presenter->getInnerDescriptorsHeap()->GetGPUDescriptorHandleForHeapStart());

    ImGui::StyleColorsClassic();

    io.Fonts->AddFontDefault();

	// Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

	static float lastTime = ImGui::GetTime();

    while (msg.message != WM_QUIT)
    {
		float deltaTime = ImGui::GetTime() - lastTime;
		lastTime = ImGui::GetTime();

		if (asLightRenderer)
		{
			// Light Updates here
			asLightRenderer->Light->Position.x = 2 * sin(ImGui::GetTime());
		}

        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application. 
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Rendering over DX12");                          // Create a window called "Hello, world!" and append into it.
			
			RenderGUI<IHasBackcolor>(technique);
			RenderGUI<IHasTriangleNumberParameter>(technique);
			RenderGUI<IHasParalellism>(technique);
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();

			{
				auto delta = ImGui::GetMouseDragDelta(1);
				if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Space)))
					camera->RotateAround(delta.x*0.01f, -delta.y*0.01f);
				else
					camera->Rotate(delta.x*0.01f, -delta.y*0.01f);
				if (delta.x != 0 || delta.y != 0)
					ImGui::ResetMouseDragDelta(1);

				if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
					camera->MoveForward(deltaTime);
				if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
					camera->MoveBackward(deltaTime);
				if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
					camera->MoveLeft(deltaTime);
				if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
					camera->MoveRight(deltaTime);
			}
        }

        // Rendering
		presenter->Present(technique);
    }

	presenter->Close();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyWindow(hWnd);
    UnregisterClass(_T("CA4G_Samples_Window"), wc.hInstance);

    return 0;
}
