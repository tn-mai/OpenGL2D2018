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

  float enemyGenerationTimer; // 次の敵が出現するまでの時間(単位:秒).
  int score; // プレイヤーのスコア.
  Audio::SoundPtr bgm; // BGM制御用変数.
  Audio::SoundPtr seBlast;
  Audio::SoundPtr sePlayerShot;

  // 敵の出現を制御するためのデータ.
  TiledMap enemyMap;
  float mapCurrentPosX;
  float mapProcessedX;
};
bool initialize(MainScene*);
void finalize(MainScene*);
void processInput(GLFWEW::WindowRef, MainScene*);
void update(GLFWEW::WindowRef, MainScene*);
void render(GLFWEW::WindowRef, MainScene*);
