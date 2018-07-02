/**
* @file TiledMap.h
*/
#ifndef EASY_TILEDMAP_H_INCLUDED
#define EASY_TILEDMAP_H_INCLUDED
#include "Sprite.h"
#include <vector>
#include <cstdint>

/**
* 2Dタイルマップを管理する.
*
* Tiled Map EditorからエクスポートされたJSONファイルを読み込み、プログラムによって操作可能なデータを構築する.
*
* - 読み込み
*   TiledMap tm;
*   tm.Load("json_from_tme.json");
*
* - 値の取得.
*   tm.GetLayer(0)
*/
class TiledMap
{
public:
  /// TMXレイヤー.
  struct Layer
  {
    uint32_t& At(int y, int x) { return mapData[y * size.x + x]; }
    uint32_t At(int y, int x) const { return mapData[y * size.x + x]; }

    std::unique_ptr<SpriteRenderer> renderer; ///< マップ描画オブジェクト(変更禁止).
    std::vector<uint32_t> mapData; ///< マップデータ.
    glm::ivec2 size; ///< マップの縦横のデータ数(変更禁止).
    glm::vec2 baseOffset; ///< 表示位置の基本オフセット(変更禁止).
    glm::vec2 offset; ///< オフセット補正値.
    bool visible; ///< 表示・非表示を制御するフラグ. true=描画される. false=描画されない.
    bool drawable; ///< 描画データの有無(変更禁止). true=描画可能. false=描画不可.
    bool dirty; ///< 描画データの再構築を制御するフラグ. true=再構築する. false=構築済みまたは描画データなし.
    int tilesetNo; ///< 対応するタイルセットのインデックス.
    float opacity; ///< 透明度. 0.0(透明)～1.0(不透明).
  };

  /// TMXタイルセット.
  struct TileSet
  {
    glm::vec2 size; ///< タイルの縦横のピクセル数.
    int columns; ///< 横のタイル数.
    int firstId; ///< 最初の(左上の)タイルのID.
    TexturePtr image; ///< テクスチャ.
  };

  /// タイルの向きを制御するフラグ.
  enum Flags {
    FlipH = 0x8000'0000u, ///< 左右反転.
    FlipV = 0x4000'0000u, ///< 上下反転.
    Rot90 = 0x2000'0000u, ///< 90度回転.
    MaskForID = 0x1fff'ffffu, ///< データからフラグを除外してIDだけを取り出すためのマスク値.
  };

  TiledMap() = default;
  explicit TiledMap(const char* filename, uint32_t drawable = 0) { Load(filename, drawable); }
  TiledMap(const TiledMap&) = default;
  TiledMap& operator=(const TiledMap&) = default;
  ~TiledMap() = default;

  bool Load(const char* filename, uint32_t drawable = 0);
  void Update();
  void Draw(const glm::vec2&) const;
  const glm::vec2& Position() const { return position; }
  void Position(const glm::vec2& pos) { position = pos; }
  size_t LayerCount() const { return layers.size(); }
  Layer& GetLayer(int n) { return layers[n]; }
  const Layer& GetLayer(int n) const { return layers[n]; }
  size_t TileSetCount() const { return tilesets.size(); }
  const TileSet& GetTileSet(int n) const { return tilesets[n]; }

private:
  std::vector<Layer> layers;
  std::vector<TileSet> tilesets;
  glm::vec2 position;
};


#endif // EASY_TILEDMAP_H_INCLUDED
