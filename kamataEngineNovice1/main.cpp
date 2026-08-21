#include "GameScene.h"

#include "KamataEngine.h"

#include <Windows.h>

using namespace KamataEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	KamataEngine::Initialize(L"GC2C_06_タン_クンブン_AL3"); 

	GameScene* gameScene = new GameScene();
	gameScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		gameScene->Update();

		dxCommon->PreDraw();
		gameScene->Draw();
		dxCommon->PostDraw();
	}

	delete gameScene;
	gameScene = nullptr;

	KamataEngine::Finalize();
	return 0;
}
