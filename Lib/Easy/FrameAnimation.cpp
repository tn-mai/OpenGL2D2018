/**
* @file SpriteAnimation.cpp
*/
#include "FrameAnimation.h"
#include "Sprite.h"
#include <algorithm>
#include <math.h>

namespace FrameAnimation {

/**
* タイムラインを作成する.
*
* @param first タイムラインを構成するキーフレーム配列の先頭.
* @param last  タイムラインを構成するキーフレーム配列の終端.
*
* @return タイムラインオブジェクト.
*
* キーフレーム配列[first, last)からタイムラインを作成する.
* タイムライン中のキーフレームの順序は配列と同じになる.
*/
TimelinePtr CreateTimeline(const KeyFrame* first, const KeyFrame* last)
{
  TimelinePtr tl = std::make_shared<Timeline>();
  tl->data.assign(first, last);
  return tl;
}

/**
* アニメーション制御オブジェクトを作成する.
*
* @param tl タイムライン.
*
* @return アニメーション制御オブジェクト.
*/
AnimatePtr CreateAnimator(const TimelinePtr& tl)
{
  return std::make_shared<Animate>(tl);
}

/**
* コンストラクタ.
*
* tl
*/
Animate::Animate(const TimelinePtr& tl)
  : timeline(tl)
{
}

/**
* アニメーションを更新する.
*
* @param targe アニメーションの適用対象となるスプライト.
* @param delta 前回の更新からの経過時間(秒).
*/
void Animate::Update(Sprite& target, glm::f32 delta)
{
  if (!timeline) {
    return;
  }
  if (!isPause) {
    elapsedTime += delta * speed;
    const glm::f32 totalTime = timeline->data.back().time;
    if (elapsedTime < 0 || elapsedTime >= totalTime) {
      if (!isLoop) {
        elapsedTime = std::min(std::max(elapsedTime, 0.0f), totalTime);
      } else {
        elapsedTime -= std::floor(elapsedTime / totalTime) * totalTime;
      }
    }
    auto itr = std::partition_point(timeline->data.begin(), timeline->data.end(), [this](const KeyFrame& e) { return e.time <= elapsedTime; });
    keyFrameIndex = std::max(itr - timeline->data.begin() - 1, 0);
  }

  const KeyFrame& keyframe = timeline->data[keyFrameIndex];
  target.Rectangle({ keyframe.origin, keyframe.size });
}

} // namespace FrameAnimation
