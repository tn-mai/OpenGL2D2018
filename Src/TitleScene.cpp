/**
* @file TitleScene.cpp
*/
#include "TitleScene.h"
#include "MainScene.h"
#include "GameData.h"
#include "Audio.h"

/**
* タイトル画面用の構造体の初期設定を行う.
*
* @param scene     タイトル画面用構造体のポインタ.
* @param gamestate ゲーム状態を表す変数のポインタ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool initialize(TitleScene* scene)
{
  scene->bg = Sprite("Res/UnknownPlanet.png");
  scene->logo = Sprite("Res/Title.png", glm::vec3(0, 100, 0));
  scene->mode = scene->modeStart;
  scene->timer = 0.5f;
  return true;
}

/**
* タイトル画面の終了処理を行う.
*
* @param scene  タイトル画面用構造体のポインタ.
*/
void finalize(TitleScene* scene)
{
  scene->bg = Sprite();
  scene->logo = Sprite();
}

/**
* タイトル画面のプレイヤー入力を処理する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  タイトル画面用構造体のポインタ.
*/
void processInput(GLFWEW::WindowRef window, TitleScene* scene)
{
  if (scene->mode != scene->modeTitle) {
    return;
  }
  const GamePad gamepad = window.GetGamePad();
  if (gamepad.buttonDown & GamePad::A) {
    scene->mode = scene->modeNextState;
    scene->timer = 1.0f;
    Audio::Engine::Instance().Prepare("Res/Audio/Start.xwm")->Play();
  }
}

/**
* タイトル画面を更新する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  タイトル画面用構造体のポインタ.
*/
void update(GLFWEW::WindowRef window, TitleScene* scene)
{
  const float deltaTime = window.DeltaTime();

  scene->bg.Update(deltaTime);
  scene->logo.Update(deltaTime);

  if (scene->timer > 0) {
    scene->timer -= deltaTime;
    return;
  }
  if (scene->mode == scene->modeStart) {
    scene->mode = scene->modeTitle;
  } else if (scene->mode == scene->modeNextState) {
    finalize(scene);
    gamestate = gamestateMain;
    initialize(&mainScene);
  }
}

/**
* タイトル画面を描画する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  タイトル画面用構造体のポインタ.
*/
void render(GLFWEW::WindowRef window, TitleScene* scene)
{
  renderer.BeginUpdate();
  renderer.AddVertices(scene->bg);
  renderer.AddVertices(scene->logo);
  renderer.EndUpdate();
  renderer.Draw(glm::vec2(window.Width(), window.Height()));

  fontRenderer.BeginUpdate();
  if (scene->mode == scene->modeTitle) {
    fontRenderer.AddString(glm::vec2(-80, -100), "START");
  } else if (scene->mode == scene->modeNextState) {
    if (static_cast<int>(scene->timer * 10) % 2) {
      fontRenderer.AddString(glm::vec2(-80, -100), "START");
    }
  }
  fontRenderer.EndUpdate();
  fontRenderer.Draw();

  window.SwapBuffers();
}
