/**
* @file TitleScene.h
*/
#ifndef TITLESCENE_H_INCLUDED
#define TITLESCENE_H_INCLUDED
#include "GLFWEW.h"
#include "Sprite.h"

/**
* タイトル画面で使用する構造体.
*/
struct TitleScene
{
  Sprite bg;
  Sprite logo;
  const int modeStart = 0;
  const int modeTitle = 1;
  const int modeNextState = 2;
  int mode;
  float timer;
};
bool initialize(TitleScene*);
void finalize(TitleScene*);
void processInput(GLFWEW::WindowRef, TitleScene*);
void update(GLFWEW::WindowRef, TitleScene*);
void render(GLFWEW::WindowRef, TitleScene*);

#endif // TITLESCENE_H_INCLUDED