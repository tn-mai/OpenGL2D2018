/**
* @file TiledMap.cpp
*/
#include "TiledMap.h"
#include "Json.h"
#include <glm/gtc/constants.hpp>
#include <sys/stat.h>
#include <algorithm>
#include <stdio.h>
#include <windows.h>

#define LOG(str, ...) { \
  char buf[1024]; \
  snprintf(buf, 1024, str, __VA_ARGS__); \
  fprintf(stderr, buf); \
  OutputDebugString(buf); \
} (void)0

/**
* JSONファイルを読み込む.
*
* @param filename JSONファイルのパス.
* @param drawable 描画データの作成を制御するビット.
*                 0なら作成しない、1なら作成する.
*                 第0bitが0番レイヤーに、第31bitが31番レイヤーに対応する.
*                 JSONファイルに存在しないレイヤーのビットは無視される.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool TiledMap::Load(const char* filename, uint32_t drawable)
{
  // JSONファイルを読み込む.
  struct stat st;
  if (stat(filename, &st)) {
    LOG("ERROR: %sを開けません.\n", filename);
    return false;
  }
  FILE* fp = fopen(filename, "rb");
  std::vector<char> buf;
  buf.resize(st.st_size);
  const size_t readSize = fread(buf.data(), 1, st.st_size, fp);
  fclose(fp);
  if (readSize != st.st_size) {
    LOG("ERROR: %sを読み込めません.\n", filename);
    return false;
  }
  Json::Result map = Json::Parse(&buf[0], &buf[0] + buf.size());
  if (!map.error.empty()) {
    LOG("ERROR: %sはデータに不備があるか、JSONファイルではありません.\n", filename);
    return false;
  }

  // マップ情報を取得.
  const Json::Object& obj = map.value.AsObject();
  const Json::Array& layerArray = obj.find("layers")->second.AsArray();
  layers.reserve(layerArray.size());
  for (const auto& e : layerArray) {
    const Json::Object& layer = e.AsObject();
    const Json::String& type = layer.find("type")->second.AsString();
    if (type != "tilelayer") {
      continue;
    }
    layers.emplace_back();
    Layer& tmp = layers.back();
    tmp.size.x = layer.find("width")->second.AsNumber<int>();
    tmp.size.y = layer.find("height")->second.AsNumber<int>();
    const auto offsetx = layer.find("offsetx");
    if (offsetx != layer.end()) {
      tmp.baseOffset.x = offsetx->second.AsNumber<float>();
    }
    const auto offsety = layer.find("offsety");
    if (offsety != layer.end()) {
      tmp.baseOffset.y = offsety->second.AsNumber<float>();
    }
    const auto opacity = layer.find("opacity");
    if (opacity != layer.end()) {
      tmp.opacity = opacity->second.AsNumber<float>();
    } else {
      tmp.opacity = 1;
    }
    tmp.visible = layer.find("visible")->second.AsBoolean();
    const Json::Array& mapArray = layer.find("data")->second.AsArray();
    tmp.mapData.reserve(mapArray.size());
    for (const auto& e : mapArray) {
      tmp.mapData.push_back(e.AsNumber<uint32_t>());
    }
    tmp.drawable = drawable & 1;
    drawable >>= 1;
    tmp.dirty = true;
  }

  // タイル情報を取得.
  const Json::Array& tilesetArray = obj.find("tilesets")->second.AsArray();
  tilesets.reserve(tilesetArray.size());
  for (const auto& e : tilesetArray) {
    const Json::Object& tileset = e.AsObject();
    tilesets.emplace_back();
    TileSet& tmp = tilesets.back();

    tmp.size.x = tileset.find("tilewidth")->second.AsNumber<float>();
    tmp.size.y = tileset.find("tileheight")->second.AsNumber<float>();
    tmp.columns = tileset.find("columns")->second.AsNumber<int>();
    tmp.firstId = tileset.find("firstgid")->second.AsNumber<int>();
    const std::string texFilename = std::string("Res/") + tileset.find("image")->second.AsString();
    tmp.image = Texture::LoadAndCache(texFilename.c_str());
    if (!tmp.image) {
      LOG("WARNING: %sを読み込めません.\n", texFilename.c_str());
    }
  }

  // タイルセットNOを設定.
  for (auto& e : layers) {
    e.tilesetNo = 0;
    const auto itr = std::find_if(e.mapData.begin(), e.mapData.end(), [](uint32_t id) { return id != 0; });
    if (itr == e.mapData.end()) {
      continue;
    }
    const int id = *itr & MaskForID;
    const auto itrTileset = std::find_if(tilesets.rbegin(), tilesets.rend(), [id](const TileSet& tileset) { return tileset.firstId <= id; });
    if (itrTileset == tilesets.rend()) {
      continue;
    }
    e.tilesetNo = (itrTileset + 1).base() - tilesets.begin();
  }

  Update(); // 描画データを作成.

  return true;
}

/**
* タイルマップを更新する.
*/
void TiledMap::Update()
{
  // 描画データの更新.
  for (auto& e : layers) {
    if (!e.dirty) {
      continue;
    }
    e.dirty = false;
    // 描画ビットが立っていなければ描画用データは作らない.
    if (!e.drawable) {
      continue;
    }
    // マップ情報から描画データを構築.
    if (!e.renderer) {
      e.renderer = std::make_unique<SpriteRenderer>();
      e.renderer->Initialize(e.size.x * e.size.y);
    }
    const TileSet& tileset = tilesets[e.tilesetNo];
    Sprite sprite(tilesets[e.tilesetNo].image);
    sprite.Color(glm::vec4(1, 1, 1, e.opacity));
    e.renderer->BeginUpdate();
    for (int y = 0; y < e.size.y; ++y) {
      for (int x = 0; x < e.size.x; ++x) {
        const uint32_t tmp = e.mapData[y * e.size.x + x];
        const int tileId = static_cast<int>(tmp & MaskForID) - tileset.firstId;
        if (tileId < 0) {
          continue;
        }
        glm::vec2 scale(1, 1);
        float rot = 0;
        switch (tmp & ~MaskForID) {
        case FlipH: scale.x = -1; break;
        case FlipV: scale.y = -1; break;
        case Rot90: scale.x = -1; rot = -glm::pi<float>() * 0.5f; break;
        case FlipH | FlipV: scale.x = -1; scale.y = -1; break;
        case FlipH | Rot90: rot = -glm::pi<float>() * 0.5f; break;
        case FlipV | Rot90: rot = glm::pi<float>() * 0.5f; break;
        }
        sprite.Scale(scale);
        sprite.Rotation(rot);
        const glm::vec2 tileOrigin((tileId % tileset.columns) * tileset.size.x, tileset.image->Height() - (tileId / tileset.columns + 1) * tileset.size.y);
        sprite.Rectangle(Rect(tileOrigin, tileset.size));
        sprite.Position(glm::vec3((glm::vec2(x + 0.5f, e.size.y - y - 0.5f) - glm::vec2(e.size) * 0.5f) * tileset.size, 0));
        sprite.Update(0);
        e.renderer->AddVertices(sprite);
      }
    }
    e.renderer->EndUpdate();
  }

  // カメラ座標の更新.
  for (auto& e : layers) {
    if (!e.drawable || !e.visible) {
      continue;
    }
    e.renderer->CameraPosition(glm::vec3(-(position + e.baseOffset + e.offset), 0));
  }
}

/**
* タイルマップを表示する.
*
* @param ss 画面の大きさ.
*/
void TiledMap::Draw(const glm::vec2& ss) const
{
  for (const auto& e : layers) {
    if (!e.drawable || !e.visible) {
      continue;
    }
    const_cast<SpriteRenderer&>(*e.renderer).CameraPosition(glm::vec3(-(position + e.baseOffset + e.offset), 0));
    e.renderer->Draw(ss);
  }
}
