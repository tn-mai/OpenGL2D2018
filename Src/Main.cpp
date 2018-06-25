/**
* @file Main.cpp
*/
#include "GLFWEW.h"
#include "Texture.h"
#include "Sprite.h"
#include "Font.h"
#include <random>

const char title[] = "OpenGL2D 2018"; // ウィンドウタイトル.
const int windowWidth = 800; // ウィンドウの幅.
const int windowHeight = 600; // ウィンドウの高さ.

/**
* ゲームキャラクター構造体.
*/
struct Actor
{
  Sprite spr; // スプライト.
  Rect collisionShape; // 衝突判定の位置と大きさ.
  int health; // 耐久力.
};

/*
* ゲームの表示に関する変数.
*/
SpriteRenderer renderer; // スプライト描画用変数.
Sprite sprBackground; // 背景用スプライト.
Sprite sprPlayer;     // 自機用スプライト.
glm::vec3 playerVelocity; // 自機の移動速度.
FrameAnimation::TimelinePtr tlEnemy; // 敵のアニメーション.
FrameAnimation::TimelinePtr tlBlast; // 爆発のアニメーション.
Actor enemyList[128]; // 敵のリスト.
Actor playerBulletList[128]; // 自機の弾のリスト.
Actor blastList[128]; // 爆発のリスト.

Font::Renderer fontRenderer; // フォント描画用変数.

/*
* ゲームのルールに関する変数.
*/
std::mt19937 random; // 乱数を発生させる変数(乱数エンジン).
float enemyGenerationTimer; // 次の敵が出現するまでの時間(単位:秒).
int score; // プレイヤーのスコア.

/*
* プロトタイプ宣言.
*/
void processInput(GLFWEW::WindowRef);
void update(GLFWEW::WindowRef);
void render(GLFWEW::WindowRef);
bool detectCollision(const Rect* lhs, const Rect* rhs);

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
  if (!Texture::Initialize()) {
    return 1;
  }
  if (!renderer.Initialize(1024)) {
    return 1;
  }

  random.seed(std::random_device()());
  fontRenderer.Init(1024, glm::vec2(windowWidth, windowHeight));
  fontRenderer.LoadFromFile("Res/Font/makinas_scrap.fnt");

  static const FrameAnimation::KeyFrame enemyAnime[] = {
    { 0.000f, glm::vec2(480, 0), glm::vec2(32, 32) },
    { 0.125f, glm::vec2(480, 96), glm::vec2(32, 32) },
    { 0.250f, glm::vec2(480, 64), glm::vec2(32, 32) },
    { 0.375f, glm::vec2(480, 32), glm::vec2(32, 32) },
    { 0.500f, glm::vec2(480, 0), glm::vec2(32, 32) },
  };
  tlEnemy = FrameAnimation::Timeline::Create(enemyAnime);

  static const FrameAnimation::KeyFrame blastAnime[] = {
    { 0.000f, glm::vec2(416,  0), glm::vec2(32, 32) },
    { 0.125f, glm::vec2(416, 32), glm::vec2(32, 32) },
    { 0.250f, glm::vec2(416, 64), glm::vec2(32, 32) },
    { 0.375f, glm::vec2(416, 96), glm::vec2(32, 32) },
    { 0.500f, glm::vec2(416, 96), glm::vec2(0, 0) },
  };
  tlBlast = FrameAnimation::Timeline::Create(blastAnime);

  // 使用する画像を用意.
  sprBackground = Sprite("Res/UnknownPlanet.png");
  sprPlayer = Sprite("Res/Objects.png", glm::vec3(0, 0, 0), Rect(0, 0, 64, 32));

  for (Actor* i = std::begin(enemyList); i != std::end(enemyList); ++i) {
    i->health = 0;
  }
  for (Actor* i = std::begin(playerBulletList); i != std::end(playerBulletList); ++i) {
    i->health = 0;
  }
  for (Actor* i = std::begin(blastList); i != std::end(blastList); ++i) {
    i->health = 0;
  }

  // ゲームループ.
  while (!window.ShouldClose()) {
    processInput(window);
    update(window);
    render(window);
  }

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

  // 自機の速度を設定する.
  const GamePad gamepad = window.GetGamePad();
  if (gamepad.buttons & GamePad::DPAD_UP) {
    playerVelocity.y = 1;
  } else if (gamepad.buttons & GamePad::DPAD_DOWN) {
    playerVelocity.y = -1;
  } else {
    playerVelocity.y = 0;
  }
  if (gamepad.buttons & GamePad::DPAD_RIGHT) {
    playerVelocity.x = 1;
  } else if (gamepad.buttons & GamePad::DPAD_LEFT) {
    playerVelocity.x = -1;
  } else {
    playerVelocity.x = 0;
  }
  if (playerVelocity.x || playerVelocity.y) {
    playerVelocity = glm::normalize(playerVelocity) * 400.0f;
  }

  // 弾の発射.
  if (gamepad.buttonDown & GamePad::A) {
    Actor* bullet = nullptr;
    for (Actor* i = std::begin(playerBulletList); i != std::end(playerBulletList); ++i) {
      if (i->health <= 0) {
        bullet = i;
        break;
      }
    }
    bullet->spr = Sprite("Res/Objects.png", sprPlayer.Position(), Rect(64, 0, 32, 16));
    bullet->spr.Tweener(TweenAnimation::Animate::Create(TweenAnimation::MoveBy::Create(1, glm::vec3(1200, 0, 0))));
    bullet->collisionShape = Rect(-16, -8, 32, 16);
    bullet->health = 1;
  }
}

/**
* ゲームの状態を更新する.
*
* @param window ゲームを管理するウィンドウ.
*/
void update(GLFWEW::WindowRef window)
{
  const float deltaTime = window.DeltaTime();

  // 自機の移動.
  if (playerVelocity.x || playerVelocity.y) {
    glm::vec3 newPos = sprPlayer.Position() + playerVelocity * deltaTime;
    const Rect playerRect = sprPlayer.Rectangle();
    if (newPos.x < -0.5f * (windowWidth - playerRect.size.x)) {
      newPos.x = -0.5f * (windowWidth - playerRect.size.x);
    } else if (newPos.x > 0.5f * (windowWidth - playerRect.size.x)) {
      newPos.x = 0.5f * (windowWidth - playerRect.size.x);
    }
    if (newPos.y < -0.5f * (windowHeight - playerRect.size.y)) {
      newPos.y = -0.5f * (windowHeight - playerRect.size.y);
    } else if (newPos.y > 0.5f * (windowHeight - playerRect.size.y)) {
      newPos.y = 0.5f * (windowHeight - playerRect.size.y);
    }
    sprPlayer.Position(newPos);
  }
  sprPlayer.Update(deltaTime);

  // 敵の出現.
  enemyGenerationTimer -= deltaTime;
  if (enemyGenerationTimer < 0) {
    Actor* enemy = nullptr;
    for (Actor* i = std::begin(enemyList); i != std::end(enemyList); ++i) {
      if (i->health <= 0) {
        enemy = i;
        break;
      }
    }
    if (enemy) {
      const std::uniform_real_distribution<float> y_distribution(-0.5f * windowHeight, 0.5f * windowHeight);
      enemy->spr = Sprite("Res/Objects.png", glm::vec3(0.5f * windowWidth, y_distribution(random), 0), Rect(480, 0, 32, 32));
      enemy->spr.Animator(FrameAnimation::Animate::Create(tlEnemy));
      enemy->spr.Tweener(TweenAnimation::Animate::Create(TweenAnimation::MoveBy::Create(5, glm::vec3(-1000, 0, 0))));
      enemy->collisionShape = Rect(-16, -16, 32, 32);
      enemy->health = 1;
      enemyGenerationTimer = 2;
    }
  }

  // 敵の更新.
  for (Actor* enemy = std::begin(enemyList); enemy != std::end(enemyList); ++enemy) {
    if (enemy->health > 0) {
      enemy->spr.Update(deltaTime);
      if (enemy->spr.Tweener()->IsFinished()) {
        enemy->health = 0;
      }
    }
  }

  // 自機の弾の更新.
  for (auto bullet = std::begin(playerBulletList); bullet != std::end(playerBulletList); ++bullet) {
    // 生きていたら状態を更新.
    if (bullet->health > 0) {
      bullet->spr.Update(deltaTime);
      if (bullet->spr.Tweener()->IsFinished()) {
        bullet->health = 0;
      }
    }
  }

  // 自機の弾と敵の衝突判定.
  for (Actor* bullet = std::begin(playerBulletList); bullet != std::end(playerBulletList); ++bullet) {
    if (bullet->health <= 0) {
      continue;
    }
    Rect shotRect = bullet->collisionShape;
    shotRect.origin += glm::vec2(bullet->spr.Position());
    for (Actor* enemy = std::begin(enemyList); enemy != std::end(enemyList); ++enemy) {
      if (enemy->health <= 0) {
        continue;
      }
      Rect enemyRect = enemy->collisionShape;
      enemyRect.origin += glm::vec2(enemy->spr.Position());
      if (detectCollision(&shotRect, &enemyRect)) {
        bullet->health -= 1;
        enemy->health -= 1;
        score += 100;
        // 爆発エフェクトを発生させる.
        Actor* blast = nullptr;
        for (Actor* i = std::begin(blastList); i != std::end(blastList); ++i) {
          if (i->health <= 0) {
            blast = i;
            break;
          }
        }
        blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
        blast->spr.Animator(FrameAnimation::Animate::Create(tlBlast));
        blast->spr.Animator()->Loop(false);
        blast->health = 1;
        break;
      }
    }
  }

  // 爆発の更新.
  for (Actor* blast = std::begin(blastList); blast != std::end(blastList); ++blast) {
    if (blast->health > 0) {
      blast->spr.Update(deltaTime);
      if (blast->spr.Animator()->IsFinished()) {
        blast->health = 0;
      }
    }
  }
}

/**
* ゲームの状態を描画する..
*
* @param window ゲームを管理するウィンドウ.
*/
void render(GLFWEW::WindowRef window)
{
  renderer.BeginUpdate();
  renderer.AddVertices(sprBackground);
  renderer.AddVertices(sprPlayer);
  for (const Actor* i = std::begin(enemyList); i != std::end(enemyList); ++i) {
    if (i->health > 0) {
      renderer.AddVertices(i->spr);
    }
  }
  for (const Actor* i = std::begin(playerBulletList); i != std::end(playerBulletList); ++i) {
    if (i->health > 0) {
      renderer.AddVertices(i->spr);
    }
  }
  for (const Actor* i = std::begin(blastList); i != std::end(blastList); ++i) {
    if (i->health > 0) {
      renderer.AddVertices(i->spr);
    }
  }
  renderer.EndUpdate();
  renderer.Draw({ windowWidth, windowHeight });

  fontRenderer.MapBuffer();
  std::wstring str;
  str.resize(8);
  int n = score;
  for (int i = 7; i >= 0; --i) {
    str[i] = L'0' + (n % 10);
    n /= 10;
  }
  fontRenderer.AddString(glm::vec2(-64 , 300), str.c_str());
  fontRenderer.UnmapBuffer();
  fontRenderer.Draw();

  window.SwapBuffers();
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
