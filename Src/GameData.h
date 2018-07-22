#pragma once
/**
* @file GameData.h
*/
#include "Sprite.h"
#include "Font.h"

struct TitleScene;
struct GameOverScene;
struct MainScene;

extern const int windowWidth; // ウィンドウの幅.
extern const int windowHeight; // ウィンドウの高さ.

extern SpriteRenderer renderer; // スプライト描画用変数.
extern FontRenderer fontRenderer; // フォント描画用変数.

extern const int gamestateTitle = 0;
extern const int gamestateMain = 1;
extern const int gamestateGameover = 2;
extern int gamestate; // ゲームの状態.

extern TitleScene titleScene;
extern GameOverScene gameOverScene;
extern MainScene mainScene;
