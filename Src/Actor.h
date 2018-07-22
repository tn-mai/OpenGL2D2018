#pragma once
/**
* @file Actor.h
*/
#include "Sprite.h"

/**
* ゲームキャラクター構造体.
*/
struct Actor
{
  Sprite spr; // スプライト.
  Rect collisionShape; // 衝突判定の位置と大きさ.
  int health; // 耐久力.
  int type; // 種類.
};

bool detectCollision(const Rect* lhs, const Rect* rhs);
void initializeActorList(Actor*, Actor*);
Actor* findAvailableActor(Actor*, Actor*);
void updateActorList(Actor*, Actor*, float deltaTime);
void renderActorList(const Actor* first, const Actor* last, SpriteRenderer* renderer);

using CollisionHandlerType = void(*)(Actor*, Actor*);
void detectCollision(Actor* first0, Actor* last0, Actor* first1, Actor* last1, CollisionHandlerType function);
