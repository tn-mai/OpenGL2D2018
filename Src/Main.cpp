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

/**
* 2つの長方形の衝突状態を調べる.
*
* @param lhs 長方形その1.
* @param rhs 長方形その2.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool detectCollision(const Rect* lhs, const Rect* rhs)
{
  return
    lhs->origin.x < rhs->origin.x + rhs->size.x &&
    lhs->origin.x + lhs->size.x > rhs->origin.x &&
    lhs->origin.y < rhs->origin.y + rhs->size.y &&
    lhs->origin.y + lhs->size.y > rhs->origin.y;
}

/**
* Actorの配列を初期化する.
*
* @param first 初期化対象の先頭要素のポインタ.
* @param last  初期化対象の終端要素のポインタ.
*/
void initializeActorList(Actor* first, Actor* last)
{
  for (Actor* i = first; i != last; ++i) {
    i->spr = Sprite();
    i->health = 0;
  }
}

/**
* 利用可能なのActorを取得する.
*
* @param first 検索対象の先頭要素のポインタ.
* @param last  検索対象の終端要素のポインタ.
*
* @return 利用可能なActorのポインタ.
*         利用可能なActorが見つからなければnullptr.
*
* [first, last)の範囲から、利用可能な(healthが0以下の)Actorを検索する.
*/
Actor* findAvailableActor(Actor* first, Actor* last)
{
  for (Actor* i = first; i != last; ++i) {
    if (i->health <= 0) {
      return i;
    }
  }
  return nullptr;
}

/**
* Actorの配列を更新する.
*
* @param first     更新対象の先頭要素のポインタ.
* @param last      更新対象の終端要素のポインタ.
* @param deltaTime 前回の更新からの経過時間.
*/
void updateActorList(Actor* first, Actor* last, float deltaTime)
{
  for (Actor* i = first; i != last; ++i) {
    if (i->health > 0) {
      i->spr.Update(deltaTime);
      if (i->spr.Tweener()->IsFinished()) {
        i->health = 0;
      }
    }
  }
}

/**
* Actorの配列を描画する.
*
* @param first    描画対象の先頭要素のポインタ.
* @param last     描画対象の終端要素のポインタ.
" @param renderer スプライト描画用の変数.
*/
void renderActorList(const Actor* first, const Actor* last, SpriteRenderer* renderer)
{
  for (const Actor* i = first; i != last; ++i) {
    if (i->health > 0) {
      renderer->AddVertices(i->spr);
    }
  }
}

/**
* 衝突を検出する.
*
* @param firstA    衝突させる配列Aの先頭ポインタ.
* @param lastA     衝突させる配列Aの終端ポインタ.
* @param firstB    衝突させる配列Bの先頭ポインタ.
* @param lastB     衝突させる配列Bの終端ポインタ.
* @param function  A-B間で衝突が検出されたときに実行する関数.
*/
void detectCollision(Actor* firstA, Actor* lastA, Actor* firstB, Actor* lastB, CollisionHandlerType function)
{
  for (Actor* a = firstA; a != lastA; ++a) {
    if (a->health <= 0) {
      continue;
    }
    Rect rectA = a->collisionShape;
    rectA.origin += glm::vec2(a->spr.Position());
    for (Actor* b = firstB; b != lastB; ++b) {
      if (b->health <= 0) {
        continue;
      }
      Rect rectB = b->collisionShape;
      rectB.origin += glm::vec2(b->spr.Position());
      if (detectCollision(&rectA, &rectB)) {
        function(a, b);
        if (a->health <= 0) {
          break;
        }
      }
    }
  }
}
