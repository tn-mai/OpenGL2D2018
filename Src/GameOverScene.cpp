/**
* @file GameOverScene.cpp
*/
#include "GameOverScene.h"
#include "TitleSceneh.h"
#include "GameData.h"

/**
* ゲームオーバー画面の初期設定を行う.
*
* @param scene     ゲームオーバー画面用構造体のポインタ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool initialize(GameOverScene* scene)
{
  scene->bg = Sprite("Res/UnknownPlanet.png");
  scene->timer = 0.5f; // 入力を受け付けない期間(秒).
  return true;
}

/**
* ゲームオーバー画面の終了処理を行う.
*
* @param scene  ゲームオーバー画面用構造体のポインタ.
*/
void finalize(GameOverScene* scene)
{
  scene->bg = Sprite();
}

/**
* ゲームオーバー画面のプレイヤー入力を処理する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  ゲームオーバー画面用構造体のポインタ.
*/
void processInput(GLFWEW::WindowRef window, GameOverScene* scene)
{
  if (scene->timer <= 0) {
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttonDown & GamePad::A) {
      gamestate = gamestateTitle;
      initialize(&titleScene);
      finalize(scene);
    }
  }
}

/**
* ゲームオーバー画面を更新する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  ゲームオーバー画面用構造体のポインタ.
*/
void update(GLFWEW::WindowRef window, GameOverScene* scene)
{
  const float deltaTime = window.DeltaTime();
  if (scene->timer > 0) {
    scene->timer -= deltaTime;
  }
  scene->bg.Update(deltaTime);
}

/**
* ゲームオーバー画面を更新する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  ゲームオーバー画面用構造体のポインタ.
*/
void render(GLFWEW::WindowRef window, GameOverScene* scene)
{
  renderer.BeginUpdate();
  renderer.AddVertices(scene->bg);
  renderer.EndUpdate();
  renderer.Draw(glm::vec2(window.Width(), window.Height()));

  fontRenderer.BeginUpdate();
  fontRenderer.AddString(glm::vec2(-144, 16), "GAME OVER");
  fontRenderer.EndUpdate();
  fontRenderer.Draw();

  window.SwapBuffers();
}
