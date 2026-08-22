#include "SceneManager.h"

#include "KamataEngine.h"

#include <Windows.h>

using namespace KamataEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	KamataEngine::Initialize(L"GC2C_06_タン_クンブン_AL3"); 

	SceneManager* sceneManager = new SceneManager();
	sceneManager->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		sceneManager->Update();

		dxCommon->PreDraw();
		sceneManager->Draw();
		dxCommon->PostDraw();
	}

	delete sceneManager;
	sceneManager = nullptr;

	KamataEngine::Finalize();
	return 0;
}
