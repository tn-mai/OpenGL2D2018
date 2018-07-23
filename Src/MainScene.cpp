/**
* @file MainScene.cpp
*/
#include "MainScene.h"
#include "GameOverScene.h"
#include "GameData.h"
#include <glm/gtc/constants.hpp>

// 敵のアニメーション.
const FrameAnimation::KeyFrame enemyKeyFrames[] = {
  { 0.000f, glm::vec2(480, 0), glm::vec2(32, 32) },
  { 0.125f, glm::vec2(480, 96), glm::vec2(32, 32) },
  { 0.250f, glm::vec2(480, 64), glm::vec2(32, 32) },
  { 0.375f, glm::vec2(480, 32), glm::vec2(32, 32) },
  { 0.500f, glm::vec2(480, 0), glm::vec2(32, 32) },
};

// 爆発アニメーション.
const FrameAnimation::KeyFrame blastKeyFrames[] = {
  { 0 / 60.0f, glm::vec2(416, 0), glm::vec2(32, 32) },
  { 5 / 60.0f, glm::vec2(416, 32), glm::vec2(32, 32) },
  { 10 / 60.0f, glm::vec2(416, 64), glm::vec2(32, 32) },
  { 15 / 60.0f, glm::vec2(416, 96), glm::vec2(32, 32) },
  { 20 / 60.0f, glm::vec2(416, 96), glm::vec2(32, 32) },
};

// アイテムの種類
const int itemNormalShot = 0;
const int itemLaser = 1;
const int itemScore = 2;

// 敵の種類.
const int enemyZako = 0;
const int enemyZakoWithNormalShotItem = 1;
const int enemyZakoWithLaserItem = 2;
const int enemyZakoWithScoreItem = 3;

void playerBulletAndEnemyContactHandler(Actor * bullet, Actor * enemy);
void playerAndEnemyContactHandler(Actor * player, Actor * enemy);
void playerAndItemContactHandler(Actor* player, Actor* item);

/**
* メイン画面用の構造体の初期設定を行う.
*
* @param scene     メイン画面用構造体のポインタ.
* @param gamestate ゲーム状態を表す変数のポインタ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool initialize(MainScene* scene)
{
  scene->tlEnemy = FrameAnimation::Timeline::Create(enemyKeyFrames);
  scene->tlBlast = FrameAnimation::Timeline::Create(blastKeyFrames);

  scene->sprBackground = Sprite("Res/UnknownPlanet.png");
  scene->sprPlayer.spr = Sprite("Res/Objects.png", glm::vec3(0, 0, 0), Rect(0, 0, 64, 32));
  scene->sprPlayer.collisionShape = Rect(-24, -8, 48, 16);
  scene->sprPlayer.health = 1;

  initializeActorList(std::begin(scene->enemyList), std::end(scene->enemyList));
  initializeActorList(std::begin(scene->playerBulletList), std::end(scene->playerBulletList));
  initializeActorList(std::begin(scene->effectList), std::end(scene->effectList));
  initializeActorList(std::begin(scene->itemList), std::end(scene->itemList));

  scene->score = 0;
  scene->weapon = scene->weaponNormalShot;
  scene->laserCount = -1;
  scene->prevLaserTime = 0;

  scene->enemyMap.Load("Res/EnemyMap.json");
  scene->mapCurrentPosX = scene->mapProcessedX = (float)GLFWEW::Window::Instance().Width();

  Audio::EngineRef audio = Audio::Engine::Instance();
  scene->seBlast = audio.Prepare("Res/Audio/Blast.xwm");
  scene->sePlayerShot = audio.Prepare("Res/Audio/PlayerShot.xwm");
  scene->sePlayerLaser = audio.Prepare("Res/Audio/Laser.xwm");
  scene->seItem = audio.Prepare("Res/Audio/GetItem.xwm");
  scene->bgm = audio.Prepare(L"Res/Audio/Neolith.xwm");
  scene->bgm->Play(Audio::Flag_Loop);

  return true;
}

/**
* メイン画面の終了処理を行う.
*
* @param scene  メイン画面用構造体のポインタ.
*/
void finalize(MainScene* scene)
{
  scene->bgm->Stop();
  scene->seBlast.reset();
  scene->sePlayerShot.reset();
  scene->sePlayerLaser.reset();
  scene->seItem.reset();
  scene->bgm.reset();

  scene->enemyMap.Unload();

  initializeActorList(std::begin(scene->enemyList), std::end(scene->enemyList));
  initializeActorList(std::begin(scene->playerBulletList), std::end(scene->playerBulletList));
  initializeActorList(std::begin(scene->effectList), std::end(scene->effectList));

  scene->sprBackground = Sprite();
  scene->sprPlayer.spr = Sprite();

  scene->tlEnemy.reset();
  scene->tlBlast.reset();
}

/**
* プレイヤーの入力を処理する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  メイン画面用構造体のポインタ.
*/
void processInput(GLFWEW::WindowRef window, MainScene* scene)
{
  if (scene->sprPlayer.health <= 0) {
    scene->playerVelocity = glm::vec3(0, 0, 0);
  } else {
    // 自機の速度を設定する.
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttons & GamePad::DPAD_UP) {
      scene->playerVelocity.y = 1;
    } else if (gamepad.buttons & GamePad::DPAD_DOWN) {
      scene->playerVelocity.y = -1;
    } else {
      scene->playerVelocity.y = 0;
    }
    if (gamepad.buttons & GamePad::DPAD_RIGHT) {
      scene->playerVelocity.x = 1;
    } else if (gamepad.buttons & GamePad::DPAD_LEFT) {
      scene->playerVelocity.x = -1;
    } else {
      scene->playerVelocity.x = 0;
    }
    if (scene->playerVelocity.x || scene->playerVelocity.y) {
      scene->playerVelocity = glm::normalize(scene->playerVelocity) * 400.0f;
    }

    // 弾の発射.
    if (scene->weapon == scene->weaponNormalShot) {
      if (gamepad.buttonDown & GamePad::A) {
        scene->sePlayerShot->Play();
        Actor* bullet = findAvailableActor(std::begin(scene->playerBulletList), std::end(scene->playerBulletList));
        if (bullet != nullptr) {
          bullet->spr = Sprite("Res/Objects.png", scene->sprPlayer.spr.Position(), Rect(64, 0, 32, 16));
          bullet->spr.Tweener(TweenAnimation::Animate::Create(TweenAnimation::MoveBy::Create(1, glm::vec3(1200, 0, 0))));
          bullet->collisionShape = Rect(-16, -8, 32, 16);
          bullet->health = 1;
          bullet->type = scene->weaponNormalShot;
        }
      }
    } else if (scene->weapon == scene->weaponLaser) {
      if ((gamepad.buttons & GamePad::A) && scene->laserCount == -1) {
        scene->laserCount = 0;
        scene->prevLaserTime = 1;
        scene->laserPosX = scene->sprPlayer.spr.Position().x + 32;
        scene->sePlayerLaser->Play();
      }
      if ((scene->laserCount >= 0 && scene->laserCount < 10) && scene->prevLaserTime >= (28.0f / 1600.0f)) {
        Actor* bullet = findAvailableActor(std::begin(scene->playerBulletList), std::end(scene->playerBulletList));
        if (bullet != nullptr) {
          Rect rect = Rect(112, 0, 32, 16);
          if (scene->laserCount == 0) {
            rect = Rect(128, 0, 32, 16);
          } else if (scene->laserCount == 9) {
            rect = Rect(96, 0, 32, 16);
          }
          glm::vec3 pos = scene->sprPlayer.spr.Position();
          pos.x = scene->laserPosX;
          bullet->spr = Sprite("Res/Objects.png", pos, rect);
          namespace TA = TweenAnimation;
          bullet->spr.Tweener(TA::Animate::Create(TA::MoveBy::Create(1, glm::vec3(1600, 0, 0), TA::EasingType::Linear, TA::Target::X)));
          bullet->collisionShape = Rect(-16, -8, 32, 16);
          bullet->health = 4;
          bullet->type = scene->weaponLaser;
        }
        scene->prevLaserTime = 0;
        scene->laserCount += 1;
      }
      if (scene->laserCount >= 10 && scene->prevLaserTime > 0.25f) {
        scene->prevLaserTime = 0;
        scene->laserCount = -1;
      }
    }
  }
}

/**
* ゲームの状態を更新する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  メイン画面用構造体のポインタ.
*/
void update(GLFWEW::WindowRef window, MainScene* scene)
{
  const float deltaTime = window.DeltaTime();

  // 自機が破壊されていたらゲームオーバー画面に切り替える.
  if (scene->sprPlayer.health <= 0) {
    scene->timer -= deltaTime;
    if (scene->timer <= 0) {
      finalize(scene);
      gamestate = gamestateGameover;
      initialize(&gameOverScene);
      return;
    }
  }

  // 自機の移動.
  if (scene->sprPlayer.health > 0) {
    if (scene->playerVelocity.x || scene->playerVelocity.y) {
      const GLFWEW::WindowRef window = GLFWEW::Window::Instance();
      const int windowWidth = window.Width();
      const int windowHeight = window.Height();
      glm::vec3 newPos = scene->sprPlayer.spr.Position() + scene->playerVelocity * deltaTime;
      const Rect playerRect = scene->sprPlayer.spr.Rectangle();
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
      scene->sprPlayer.spr.Position(newPos);
    }
    scene->sprPlayer.spr.Update(deltaTime);
    scene->prevLaserTime += deltaTime;
  }

  // 敵の出現.
#if 1
  const TiledMap::Layer& tiledMapLayer = scene->enemyMap.GetLayer(0);
  const glm::vec2 tileSize = scene->enemyMap.GetTileSet(tiledMapLayer.tilesetNo).size;
  // 敵配置マップ参照位置の更新.
  const float enemyMapScrollSpeed = 100; // 更新速度.
  scene->mapCurrentPosX += enemyMapScrollSpeed * deltaTime;
  if (scene->mapCurrentPosX >= tiledMapLayer.size.x * tileSize.x) {
    // 終端を超えたら先頭にループ.
    scene->mapCurrentPosX = 0;
    scene->mapProcessedX = 0;
  }
  // 次の列に到達したらデータを読む.
  if (scene->mapCurrentPosX - scene->mapProcessedX >= tileSize.x) {
    scene->mapProcessedX += tileSize.x;
    const int mapX = static_cast<int>(scene->mapProcessedX / tileSize.x);
    for (int mapY = 0; mapY < tiledMapLayer.size.y; ++mapY) {
      const int enemyId = 256; // 敵とみなすタイルID.
      const int tileNo = tiledMapLayer.At(mapY, mapX);
      struct EnemyData
      {
        int id;
        int type;
        Rect imageRect;
        Rect collisionRect;
      };
      const EnemyData enemyDataList[] = {
        { 256, enemyZako, Rect(480, 0, 32, 32), Rect(-16, -16, 32, 32) },
      { 228, enemyZakoWithNormalShotItem, Rect(480, 0, 32, 32), Rect(-16, -16, 32, 32) },
      { 229, enemyZakoWithLaserItem, Rect(480, 0, 32, 32), Rect(-16, -16, 32, 32) },
      { 230, enemyZakoWithScoreItem, Rect(480, 0, 32, 32), Rect(-16, -16, 32, 32) },
      };
      const EnemyData* enemyData = nullptr;
      for (const EnemyData* i = std::begin(enemyDataList); i != std::end(enemyDataList); ++i) {
        if (i->id == tileNo) {
          enemyData = i;
          break;
        }
      }
      if (enemyData != nullptr) {
        Actor* enemy = findAvailableActor(std::begin(scene->enemyList), std::end(scene->enemyList));
        if (enemy != nullptr) {
          const float y = window.Height() * 0.5f - static_cast<float>(mapY * tileSize.x);
          enemy->spr = Sprite("Res/Objects.png", glm::vec3(0.5f * window.Width(), y, 0), enemyData->imageRect);
          enemy->spr.Animator(FrameAnimation::Animate::Create(scene->tlEnemy));
          namespace TA = TweenAnimation;
          TA::SequencePtr seq = TA::Sequence::Create(4);
          seq->Add(TA::MoveBy::Create(1, glm::vec3(0, 100, 0), TA::EasingType::EaseInOut, TA::Target::Y));
          seq->Add(TA::MoveBy::Create(1, glm::vec3(0, -100, 0), TA::EasingType::EaseInOut, TA::Target::Y));
          TA::ParallelizePtr par = TA::Parallelize::Create(1);
          par->Add(seq);
          par->Add(TA::MoveBy::Create(8, glm::vec3(-1000, 0, 0), TA::EasingType::Linear, TA::Target::X));
          enemy->spr.Tweener(TA::Animate::Create(par));
          enemy->collisionShape = enemyData->collisionRect;
          enemy->health = 1;
          enemy->type = enemyData->type;
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
  const float y = scene->sprPlayer.spr.Position().y;
  for (Actor* i = std::begin(scene->playerBulletList); i != std::end(scene->playerBulletList); ++i) {
    if (i->health > 0 && i->type == scene->weaponLaser) {
      glm::vec3 pos = i->spr.Position();
      pos.y = y;
      i->spr.Position(pos);
    }
  }
  updateActorList(std::begin(scene->enemyList), std::end(scene->enemyList), deltaTime);
  updateActorList(std::begin(scene->playerBulletList), std::end(scene->playerBulletList), deltaTime);
  updateActorList(std::begin(scene->effectList), std::end(scene->effectList), deltaTime);
  updateActorList(std::begin(scene->itemList), std::end(scene->itemList), deltaTime);

  // 自機とアイテムの衝突判定.
  detectCollision(
    &scene->sprPlayer, &scene->sprPlayer + 1,
    std::begin(scene->itemList), std::end(scene->itemList),
    playerAndItemContactHandler
  );

  // 自機の弾と敵の衝突判定.
  detectCollision(
    std::begin(scene->playerBulletList), std::end(scene->playerBulletList),
    std::begin(scene->enemyList), std::end(scene->enemyList),
    playerBulletAndEnemyContactHandler);

  // 自機と敵の衝突判定.
  detectCollision(
    &scene->sprPlayer, &scene->sprPlayer + 1,
    std::begin(scene->enemyList), std::end(scene->enemyList),
    playerAndEnemyContactHandler
  );
}

/**
* ゲームの状態を描画する.
*
* @param window ゲームを管理するウィンドウ.
* @param scene  メイン画面用構造体のポインタ.
*/
void render(GLFWEW::WindowRef window, MainScene* scene)
{
  renderer.BeginUpdate();
  renderer.AddVertices(scene->sprBackground);
  if (scene->sprPlayer.health > 0) {
    renderer.AddVertices(scene->sprPlayer.spr);
  }
  renderActorList(std::begin(scene->enemyList), std::end(scene->enemyList), &renderer);
  renderActorList(std::begin(scene->playerBulletList), std::end(scene->playerBulletList), &renderer);
  renderActorList(std::begin(scene->effectList), std::end(scene->effectList), &renderer);
  renderActorList(std::begin(scene->itemList), std::end(scene->itemList), &renderer);
  renderer.EndUpdate();
  renderer.Draw({ window.Width(), window.Height() });

  fontRenderer.BeginUpdate();
  char str[9];
  snprintf(str, 9, "%08d", scene->score);
  fontRenderer.AddString(glm::vec2(-64, 300), str);
  fontRenderer.EndUpdate();
  fontRenderer.Draw();

  window.SwapBuffers();
}

/**
* 自機の弾と敵の衝突を処理する.
*
* @param bullet 自機の弾のポインタ.
* @param enemy  敵のポインタ.
*/
void playerBulletAndEnemyContactHandler(Actor * bullet, Actor * enemy)
{
  // レーザーは耐久値4未満の敵を貫通する.
  enemy->health -= bullet->health;
  if (bullet->type != mainScene.weaponLaser || enemy->health >= 4) {
    bullet->health = 0;
  }
  if (enemy->health <= 0) {
    mainScene.score += 100;
    mainScene.seBlast->Play();
    Actor* blast = findAvailableActor(std::begin(mainScene.effectList), std::end(mainScene.effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(mainScene.tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->health = 1;
    }
    // アイテムを持っている敵だった場合、対応するアイテムを出現させる.
    if (enemy->type >= enemyZakoWithNormalShotItem && enemy->type <= enemyZakoWithScoreItem) {
      Actor* item = findAvailableActor(std::begin(mainScene.itemList), std::end(mainScene.itemList));
      if (item != nullptr) {
        item->type = enemy->type - enemyZakoWithNormalShotItem;
        item->spr = Sprite("Res/Objects.png", enemy->spr.Position(), Rect((float)(96 + item->type * 32), 32, 32, 32));
        namespace TA = TweenAnimation;
        item->spr.Tweener(TA::Animate::Create(TA::MoveBy::Create(8, glm::vec3(-800, 0, 0))));
        item->collisionShape = Rect(-16, -16, 32, 32);
        item->health = 1;
      }
    }
  }
}

/**
* 自機の弾と敵の衝突を処理する.
*
* @param bullet 自機のポインタ.
* @param enemy  敵のポインタ.
*/
void playerAndEnemyContactHandler(Actor * player, Actor * enemy)
{
  if (player->health > enemy->health) {
    player->health -= enemy->health;
    enemy->health = 0;;
  } else {
    enemy->health -= player->health;
    player->health = 0;
  }
  if (enemy->health <= 0) {
    mainScene.score += 100;
    Actor* blast = findAvailableActor(std::begin(mainScene.effectList), std::end(mainScene.effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(mainScene.tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->health = 1;
    }
  }
  if (player->health <= 0) {
    Actor* blast = findAvailableActor(std::begin(mainScene.effectList), std::end(mainScene.effectList));
    if (blast != nullptr) {
      blast->spr = Sprite("Res/Objects.png", enemy->spr.Position());
      blast->spr.Animator(FrameAnimation::Animate::Create(mainScene.tlBlast));
      namespace TA = TweenAnimation;
      blast->spr.Tweener(TA::Animate::Create(TA::Rotation::Create(20 / 60.0f, glm::pi<float>() * 0.5f)));
      blast->spr.Scale(glm::vec2(2, 2));
      blast->health = 1;
    }
    mainScene.timer = 2;
  }
}

/**
* 自機とアイテムの衝突.
*/
void playerAndItemContactHandler(Actor* player, Actor* item)
{
  mainScene.seItem->Play();
  if (item->type == itemNormalShot) {
    mainScene.weapon = mainScene.weaponNormalShot;
  } else if (item->type == itemLaser) {
    mainScene.weapon = mainScene.weaponLaser;
    mainScene.laserCount = -1;
    mainScene.prevLaserTime = 0;
  } else if (item->type == itemScore) {
    mainScene.score += 1000;
  }
  item->health = 0;
}