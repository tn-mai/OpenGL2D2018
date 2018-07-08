/**
* @file Main.cpp
*/
#include "GLFWEW.h"
#include "Texture.h"
#include "Sprite.h"
#include "Font.h"
#include "TiledMap.h"
#include <glm/gtc/constants.hpp>
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
FontRenderer fontRenderer; // フォント描画用変数.
Sprite sprBackground; // 背景用スプライト.
Actor sprPlayer;     // 自機用スプライト.
glm::vec3 playerVelocity; // 自機の移動速度.
Actor enemyList[128]; // 敵のリスト.
Actor playerBulletList[128]; // 自機の弾のリスト.
Actor effectList[128]; // 爆発などの特殊効果用スプライトのリスト.

// 敵のアニメーション.
const FrameAnimation::KeyFrame enemyKeyFrames[] = {
  { 0.000f, glm::vec2(480, 0), glm::vec2(32, 32) },
  { 0.125f, glm::vec2(480, 96), glm::vec2(32, 32) },
  { 0.250f, glm::vec2(480, 64), glm::vec2(32, 32) },
  { 0.375f, glm::vec2(480, 32), glm::vec2(32, 32) },
  { 0.500f, glm::vec2(480, 0), glm::vec2(32, 32) },
};
FrameAnimation::TimelinePtr tlEnemy;

// 爆発アニメーション.
const FrameAnimation::KeyFrame blastKeyFrames[] = {
  { 0 / 60.0f, glm::vec2(416, 0), glm::vec2(32, 32) },
  { 5 / 60.0f, glm::vec2(416, 32), glm::vec2(32, 32) },
  { 10 / 60.0f, glm::vec2(416, 64), glm::vec2(32, 32) },
  { 15 / 60.0f, glm::vec2(416, 96), glm::vec2(32, 32) },
  { 20 / 60.0f, glm::vec2(416, 96), glm::vec2(32, 32) },
};
FrameAnimation::TimelinePtr tlBlast;

// 敵の出現を制御するためのデータ.
TiledMap enemyMap;
float mapCurrentPosX;
float mapProcessedX;

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
void initializeActorList(Actor*, Actor*);
Actor* findAvailableActor(Actor*, Actor*);
void updateActorList(Actor*, Actor*, float deltaTime);
void renderActorList(const Actor* first, const Actor* last, SpriteRenderer* renderer);

using CollisionHandlerType = bool(*)(Actor*, Actor*);
void collisionDetection(Actor* first0, Actor* last0, Actor* first1, Actor* last1, CollisionHandlerType function);
bool playerBulletAndEnemyContactHandler(Actor * bullet, Actor * enemy);
bool playerAndEnemyContactHandler(Actor * player, Actor * enemy);

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
  if (!fontRenderer.Initialize(1024, glm::vec2(windowWidth, windowHeight))) {
    return 1;
  }
  if (!fontRenderer.LoadFromFile("Res/Font/makinas_scrap.fnt")) {
    return 1;
  }

  random.seed(std::random_device()()); // 乱数エンジンの初期化.

  tlEnemy = FrameAnimation::Timeline::Create(enemyKeyFrames);
  tlBlast = FrameAnimation::Timeline::Create(blastKeyFrames);

  // 使用する画像を用意.
  sprBackground = Sprite("Res/UnknownPlanet.png");
  sprPlayer.spr = Sprite("Res/Objects.png", glm::vec3(0, 0, 0), Rect(0, 0, 64, 32));
  sprPlayer.collisionShape = Rect(-24, -8, 48, 16);
  sprPlayer.health = 1;

  initializeActorList(std::begin(enemyList), std::end(enemyList));
  initializeActorList(std::begin(playerBulletList), std::end(playerBulletList));
  initializeActorList(std::begin(effectList), std::end(effectList));
  enemyGenerationTimer = 2;

  score = 0;

  // 敵配置マップを読み込む.
  enemyMap.Load("Res/EnemyMap.json");
  mapCurrentPosX = mapProcessedX = windowWidth;

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

  if (sprPlayer.health > 0) {
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
      Actor* bullet = findAvailableActor(std::begin(playerBulletList), std::end(playerBulletList));
      if (bullet != nullptr) {
        bullet->spr = Sprite("Res/Objects.png", sprPlayer.spr.Position(), Rect(64, 0, 32, 16));
        bullet->spr.Tweener(TweenAnimation::Animate::Create(TweenAnimation::MoveBy::Create(1, glm::vec3(1200, 0, 0))));
        bullet->collisionShape = Rect(-16, -8, 32, 16);
        bullet->health = 1;
      }
    }
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
  if (sprPlayer.health > 0) {
    if (playerVelocity.x || playerVelocity.y) {
      glm::vec3 newPos = sprPlayer.spr.Position() + playerVelocity * deltaTime;
      const Rect playerRect = sprPlayer.spr.Rectangle();
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
      sprPlayer.spr.Position(newPos);
    }
    sprPlayer.spr.Update(deltaTime);
  }

  // 敵の出現.
#if 1
  const TiledMap::Layer& tiledMapLayer = enemyMap.GetLayer(0);
  const glm::vec2 tileSize = enemyMap.GetTileSet(tiledMapLayer.tilesetNo).size;
  // 敵配置マップ参照位置の更新.
  const float enemyMapScrollSpeed = 100; // 更新速度.
  mapCurrentPosX += enemyMapScrollSpeed * deltaTime;
  if (mapCurrentPosX >= tiledMapLayer.size.x * tileSize.x) {
    // 終端を超えたら先頭にループ.
    mapCurrentPosX = 0;
    mapProcessedX = 0;
  }
  // 次の列に到達したらデータを読む.
  if (mapCurrentPosX - mapProcessedX >= tileSize.x) {
    mapProcessedX += tileSize.x;
    const int mapX = static_cast<int>(mapProcessedX / tileSize.x);
    for (int mapY = 0; mapY < tiledMapLayer.size.y; ++mapY) {
      const int enemyId = 256; // 敵とみなすタイルID.
      if (tiledMapLayer.At(mapY, mapX) == enemyId) {
        Actor* enemy = findAvailableActor(std::begin(enemyList), std::end(enemyList));
        if (enemy != nullptr) {
          const float y = windowHeight * 0.5f - static_cast<float>(mapY * tileSize.x);
          enemy->spr = Sprite("Res/Objects.png", glm::vec3(0.5f * windowWidth, y, 0), Rect(480, 0, 32, 32));
          enemy->spr.Animator(FrameAnimation::Animate::Create(tlEnemy));
          namespace TA = TweenAnimation;
          TA::SequencePtr seq = TA::Sequence::Create(4);
          seq->Add(TA::MoveBy::Create(1, glm::vec3(0, 100, 0), TA::EasingType::EaseInOut, TA::Target::Y));
          seq->Add(TA::MoveBy::Create(1, glm::vec3(0, -100, 0), TA::EasingType::EaseInOut, TA::Target::Y));
          TA::ParallelizePtr par = TA::Parallelize::Create(1);
          par->Add(seq);
          par->Add(TA::MoveBy::Create(8, glm::vec3(-1000, 0, 0), TA::EasingType::Linear, TA::Target::X));
          enemy->spr.Tweener(TA::Animate::Create(par));
          enemy->collisionShape = Rect(-16, -16, 32, 32);
          enemy->health = 1;
        }
      }
    }
  }
#else
  enemyGenerationTimer -= deltaTime;
  if (enemyGenerationTimer < 0) {
    Actor* enemy = FindAvailableActor(std::begin(enemyList), std::end(enemyList));
    if (enemy != nullptr) {
      const std::uniform_real_distribution<float> y_distribution(-0.5f * windowHeight, 0.5f * windowHeight);
      enemy->spr = Sprite("Res/Objects.png", glm::vec3(0.5f * windowWidth, y_distribution(random), 0), Rect(480, 0, 32, 32));
      enemy->spr.Animator(FrameAnimation::Animate::Create(tlEnemy));
      namespace TA = TweenAnimation;
      TA::SequencePtr seq = TA::Sequence::Create(2);
      seq->Add(TA::MoveBy::Create(1, glm::vec3(-200, 100, 0), TA::EasingType::Linear));
      seq->Add(TA::MoveBy::Create(1, glm::vec3(-200, -100, 0), TA::EasingType::Linear));
      enemy->spr.Tweener(TweenAnimation::Animate::Create(seq));
      enemy->collisionShape = Rect(-16, -16, 32, 32);
      enemy->health = 1;
      enemyGenerationTimer = 2;
    }
  }
#endif

  // Actorの更新.
  updateActorList(std::begin(enemyList), std::end(enemyList), deltaTime);
  updateActorList(std::begin(playerBulletList), std::end(playerBulletList), deltaTime);
  updateActorList(std::begin(effectList), std::end(effectList), deltaTime);

  // 自機の弾と敵の衝突判定.
  collisionDetection(
    std::begin(playerBulletList), std::end(playerBulletList),
    std::begin(enemyList), std::end(enemyList),
    playerBulletAndEnemyContactHandler);

  // 自機と敵の衝突判定.
  collisionDetection(
    &sprPlayer, &sprPlayer + 1,
    std::begin(enemyList), std::end(enemyList),
    playerAndEnemyContactHandler
  );
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
  if (sprPlayer.health > 0) {
    renderer.AddVertices(sprPlayer.spr);
  }
  renderActorList(std::begin(enemyList), std::end(enemyList), &renderer);
  renderActorList(std::begin(playerBulletList), std::end(playerBulletList), &renderer);
  renderActorList(std::begin(effectList), std::end(effectList), &renderer);
  renderer.EndUpdate();
  renderer.Draw({ windowWidth, windowHeight });

  fontRenderer.BeginUpdate();
  char str[9];
  snprintf(str, 9, "%08d", score);
  fontRenderer.AddString(glm::vec2(-64 , 300), str);
  fontRenderer.EndUpdate();
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

/**
* Actorの配列を初期化する.
*
* @param first 初期化対象の先頭要素のポインタ.
* @param last  初期化対象の終端要素のポインタ.
*/
void initializeActorList(Actor* first, Actor* last)
{
  for (Actor* i = first; i != last; ++i) {
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
* 自機の弾と敵の衝突を処理する.
*
* @param bullet 自機の弾のポインタ.
* @param enemy  敵のポインタ.
*
* @retval true  弾の衝突処理を終了する.
* @retval false 弾の衝突処理を継続する.
*/
bool playerBulletAndEnemyContactHandler(Actor * bullet, Actor * enemy)
{
  bullet->health -= 1;
  enemy->health -= 1;
  if (enemy->health <= 0) {
    score += 100;
    Actor* blast = findAvailableActor(std::begin(effectList), std::end(effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->health = 1;
    }
  }
  return bullet->health <= 0;
}

/**
* 自機の弾と敵の衝突を処理する.
*
* @param bullet 自機のポインタ.
* @param enemy  敵のポインタ.
*
* @retval true  弾の衝突処理を終了する.
* @retval false 弾の衝突処理を継続する.
*/
bool playerAndEnemyContactHandler(Actor * player, Actor * enemy)
{
  if (player->health > enemy->health) {
    player->health -= enemy->health;
    enemy->health = 0;;
  } else {
    enemy->health -= player->health;
    player->health = 0;
  }
  if (enemy->health <= 0) {
    score += 100;
    Actor* blast = findAvailableActor(std::begin(effectList), std::end(effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->health = 1;
    }
  }
  if (player->health <= 0) {
    Actor* blast = findAvailableActor(std::begin(effectList), std::end(effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->spr.Scale(glm::vec2(2, 2));
      blast->health = 1;
    }
  }
  return player->health <= 0;
}

/**
* 衝突判定.
*/
void collisionDetection(Actor* first0, Actor* last0, Actor* first1, Actor* last1, CollisionHandlerType function)
{
  for (Actor* bullet = first0; bullet != last0; ++bullet) {
    if (bullet->health <= 0) {
      continue;
    }
    Rect shotRect = bullet->collisionShape;
    shotRect.origin += glm::vec2(bullet->spr.Position());
    for (Actor* enemy = first1; enemy != last1; ++enemy) {
      if (enemy->health <= 0) {
        continue;
      }
      Rect enemyRect = enemy->collisionShape;
      enemyRect.origin += glm::vec2(enemy->spr.Position());
      if (detectCollision(&shotRect, &enemyRect)) {
        if (function(bullet, enemy)) {
          break;
        }
      }
    }
  }
}