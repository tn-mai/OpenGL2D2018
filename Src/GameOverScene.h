#pragma once
/**
* @file GameOverScene.h
*/
#include "GLFWEW.h"
#include "Sprite.h"

/**
* ゲームオーバー画面で使用する構造体.
*/
struct GameOverScene
{
  Sprite bg;
  float timer;
};
bool initialize(GameOverScene*);
void finalize(GameOverScene*);
void processInput(GLFWEW::WindowRef, GameOverScene*);
void update(GLFWEW::WindowRef, GameOverScene*);
void render(GLFWEW::WindowRef, GameOverScene*);