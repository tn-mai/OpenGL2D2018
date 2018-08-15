#pragma once
/**
* @file MainScene.h
*/
#include "GLFWEW.h"
#include "Sprite.h"
#include "Audio.h"
#include "TiledMap.h"
#include "Actor.h"

/**
* メイン画面で使用する構造体.
*/
struct MainScene
{
  FrameAnimation::TimelinePtr tlEnemy;
  FrameAnimation::TimelinePtr tlBlast;

  Sprite sprBackground; // 背景用スプライト.
  Actor sprPlayer;     // 自機用スプライト.
  glm::vec3 playerVelocity; // 自機の移動速度.
  Actor enemyList[128]; // 敵のリスト.
  Actor playerBulletList[128]; // 自機の弾のリスト.
  Actor effectList[128]; // 爆発などの特殊効果用スプライトのリスト.
  Actor itemList[32]; // パワーアップや得点アイテムのリスト.

  float enemyGenerationTimer; // 次の敵が出現するまでの時間(単位:秒).
  int score; // プレイヤーのスコア.

  // 武器の種類.
  const int weaponNormalShot = 0;
  const int weaponLaser = 1;

  int weapon; // 自機の武器.

  const int weaponLevelMin = 0;
  const int weaponLevelMax = 5;
  int weaponLevel; // 武器の強化度.

  float shotTimer; // 弾の発射間隔.

  const int laserLength = 16;
  int laserCount; // 発射中のレーザーの長さ.
  Actor* laserBack; // レーザーの末尾.
  float laserPosX; // レーザー発射地点のX座標(Y座標は自機に追随するので不要).

  Audio::SoundPtr bgm; // BGM制御用変数.
  Audio::SoundPtr seBlast;
  Audio::SoundPtr sePlayerShot;
  Audio::SoundPtr sePlayerLaser;
  Audio::SoundPtr seItem;

  // 敵の出現を制御するためのデータ.
  TiledMap enemyMap;
  float mapCurrentPosX;
  float mapProcessedX;

  float timer;
};
bool initialize(MainScene*);
void finalize(MainScene*);
void processInput(GLFWEW::WindowRef, MainScene*);
void update(GLFWEW::WindowRef, MainScene*);
void render(GLFWEW::WindowRef, MainScene*);
