/**
* @file Main.cpp
*/
#include "TitleSceneh.h"
#include "GameOverScene.h"
#include "MainScene.h"
#include "GameData.h"
#include "GLFWEW.h"
#include "Texture.h"
#include "Sprite.h"
#include "Font.h"
#include "TiledMap.h"
#include "Audio.h"
#include <glm/gtc/constants.hpp>
#include <random>

const char title[] = "OpenGL2D 2018"; // ウィンドウタイトル.
const int windowWidth = 800; // ウィンドウの幅.
const int windowHeight = 600; // ウィンドウの高さ.

/*
* ゲームの表示に関する変数.
*/
SpriteRenderer renderer; // スプライト描画用変数.
FontRenderer fontRenderer; // フォント描画用変数.

/*
* ゲームのルールに関する変数.
*/
std::mt19937 random; // 乱数を発生させる変数(乱数エンジン).

/*
* プロトタイプ宣言.
*/
void processInput(GLFWEW::WindowRef);
void update(GLFWEW::WindowRef);
void render(GLFWEW::WindowRef);

int gamestate; // ゲームの状態.

TitleScene titleScene;
GameOverScene gameOverScene;
MainScene mainScene;

/**
* プログラムのエントリーポイント.
*/
int main()
{
  // アプリケーションの初期化.
  GLFWEW::WindowRef window = GLFWEW::Window::Instance();
  if (!window.Initialize(windowWidth, windowHeight, title)) {
    return 1;
  }
  Audio::EngineRef audio = Audio::Engine::Instance();
  if (!audio.Initialize()) {
    return 1;
  }
  if (!Texture::Initialize()) {
    return 1;
  }
  if (!renderer.Initialize(1024)) {
    return 1;
  }
  if (!fontRenderer.Initialize(1024, glm::vec2(windowWidth, windowHeight))) {
    return 1;
  }
  if (!fontRenderer.LoadFromFile("Res/Font/makinas_scrap.fnt")) {
    return 1;
  }

  random.seed(std::random_device()()); // 乱数エンジンの初期化.

  initialize(&titleScene);

  // ゲームループ.
  while (!window.ShouldClose()) {
    processInput(window);
    update(window);
    render(window);
    audio.Update();
  }

  audio.Destroy();
  Texture::Finalize();
  return 0;
}

/**
* プレイヤーの入力を処理する.
*
* @param window ゲームを管理するウィンドウ.
*/
void processInput(GLFWEW::WindowRef window)
{
  window.Update();

  if (gamestate == gamestateTitle) {
    processInput(window, &titleScene);
    return;
  } else if (gamestate == gamestateGameover) {
    processInput(window, &gameOverScene);
    return;
  } else if (gamestate == gamestateMain) {
    processInput(window, &mainScene);
  }
}

/**
* ゲームの状態を更新する.
*
* @param window ゲームを管理するウィンドウ.
*/
void update(GLFWEW::WindowRef window)
{
  if (gamestate == gamestateTitle) {
    update(window, &titleScene);
    return;
  } else if (gamestate == gamestateGameover) {
    update(window, &gameOverScene);
    return;
  } else if (gamestate == gamestateMain) {
    update(window, &mainScene);
    if (mainScene.sprPlayer.health <= 0) {
      finalize(&mainScene);
      gamestate = gamestateGameover;
      initialize(&gameOverScene);
      return;
    }
  }
}

/**
* ゲームの状態を描画する.
*
* @param window ゲームを管理するウィンドウ.
*/
void render(GLFWEW::WindowRef window)
{
  if (gamestate == gamestateTitle) {
    render(window, &titleScene);
    return;
  } else if (gamestate == gamestateGameover) {
    render(window, &gameOverScene);
    return;
  } else if (gamestate == gamestateMain) {
    render(window, &mainScene);
    return;
  }
}
