/**
* @file Font.cpp
*/
#include "Font.h"
#include <memory>
#include <iostream>
#include <stdio.h>

/**
* フォント描画機能を格納する名前空間.
*/
namespace Font {

/**
* フォント用頂点データ型.
*/
struct Vertex
{
  glm::vec3 position;
  glm::u16vec2 uv;
  glm::u8vec4 color;
  glm::u8vec4 subColor;
  glm::u16vec2 thicknessAndOutline;
};

/**
* フォント描画オブジェクトを初期化する.
*
* @param maxChar 最大描画文字数.
* @param screen  描画先スクリーンの大きさ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool Renderer::Initialize(size_t maxChar, const glm::vec2& screen)
{
  if (maxChar > (USHRT_MAX + 1) / 4) {
    std::cerr << "WARNING: " << maxChar << "は設定可能な最大文字数を越えています"<< std::endl;
    maxChar = (USHRT_MAX + 1) / 4;
  }
  vboCapacity = static_cast<GLsizei>(4 * maxChar);
  vbo.Init(GL_ARRAY_BUFFER, sizeof(Vertex) * vboCapacity, nullptr, GL_STREAM_DRAW);
  {
    std::vector<GLushort> tmp;
    tmp.resize(maxChar * 6);
    GLushort* p = tmp.data();
    for (GLushort i = 0; i < maxChar * 4; i += 4) {
      for (GLshort n : { 0, 1, 2, 2, 3, 0 }) {
        *(p++) = i + n;
      }
    }
    ibo.Init(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 6 * maxChar, tmp.data(), GL_STATIC_DRAW);
  }
  vao.Init(vbo.Id(), ibo.Id());
  vao.Bind();
  vao.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
  vao.VertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(Vertex), offsetof(Vertex, uv));
  vao.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, color));
  vao.VertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, subColor));
  vao.VertexAttribPointer(4, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(Vertex), offsetof(Vertex, thicknessAndOutline));
  vao.Unbind();

  progFont = Shader::Program::Create("Res/Shader/Font.vert", "Res/Shader/Font.frag");
  if (!progFont) {
    return false;
  }

  reciprocalScreenSize = 2.0f / screen;
  setlocale(LC_CTYPE, "JPN");
  return true;
}

/**
* フォントファイルを読み込む.
*
* @param filename フォントファイル名.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool Renderer::LoadFromFile(const char* filename)
{
  const std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(filename, "r"), fclose);
  if (!fp) {
    return false;
  }

  int line = 1;
  float fontSize;
  int ret = fscanf(fp.get(), "info face=\"%*[^\"]\" size=%f bold=%*d italic=%*d charset=%*s"
    " unicode=%*d stretchH=%*d smooth=%*d aa=%*d padding=%*d,%*d,%*d,%*d spacing=%*d,%*d%*[^\r\n]", &fontSize);
  ++line;
  const float reciprocalFontSize = 1.0f / fontSize;

  glm::vec2 scale;
  ret = fscanf(fp.get(), " common lineHeight=%*d base=%*d scaleW=%f scaleH=%f pages=%*d packed=%*d%*[^\r\n]", &scale.x, &scale.y);
  if (ret < 2) {
    std::cerr << "ERROR: '" << filename << "'の読み込みに失敗(line=" << line << ")" << std::endl;
    return false;
  }
  const glm::vec2 reciprocalScale = glm::vec2(1.0f / scale);
  ++line;

  std::vector<std::string> texNameList;
  for (;;) {
    char tex[128];
    ret = fscanf(fp.get(), " page id=%*d file=%127s", tex);
    if (ret < 1) {
      break;
    }
    std::string texFilename = filename;
    const size_t lastSlashIndex = texFilename.find_last_of('/', std::string::npos);
    if (lastSlashIndex == std::string::npos) {
      texFilename.clear();
    } else {
      texFilename.resize(lastSlashIndex + 1);
    }
    texFilename.append(tex + 1); // 最初の「"」を抜いて追加.
    texFilename.pop_back(); // 最後の「"」を消す.
    texNameList.push_back(texFilename);
    ++line;
  }
  if (texNameList.empty()) {
    std::cerr << "ERROR: '" << filename << "'の読み込みに失敗(line=" << line << ")" << std::endl;
    return false;
  }

  int charCount;
  ret = fscanf(fp.get(), " chars count=%d", &charCount);
  if (ret < 1) {
    std::cerr << "ERROR: '" << filename << "'の読み込みに失敗(line=" << line << ")" << std::endl;
    return false;
  }
  ++line;

  fixedAdvance = 0;
  fontList.resize(65536);
  for (int i = 0; i < charCount; ++i) {
    FontInfo font;
    glm::vec2 uv;
    ret = fscanf(fp.get(), " char id=%d x=%f y=%f width=%f height=%f xoffset=%f yoffset=%f xadvance=%f page=%d chnl=%*d", &font.id, &uv.x, &uv.y, &font.size.x, &font.size.y, &font.offset.x, &font.offset.y, &font.xadvance, &font.page);
    if (ret < 9) {
      std::cerr << "ERROR: '" << filename << "'の読み込みに失敗(line=" << line << ")" << std::endl;
      return false;
    }
    font.offset.y *= -1;
    uv.y = scale.y - uv.y - font.size.y;
    font.uv[0] = uv * reciprocalScale * 65535.0f;
    font.uv[1] = (uv + font.size) * reciprocalScale * 65535.0f;
    if (font.id < 65536) {
      fontList[font.id] = font;
      if (font.xadvance > fixedAdvance) {
        fixedAdvance = font.xadvance;
      }
    }
    ++line;
  }
  texList.clear();
  texList.reserve(texNameList.size());
  for (const auto& e : texNameList) {
    TexturePtr tex = Texture::LoadFromFile(e.c_str());
    if (!tex) {
      return false;
    }
    texList.push_back(tex);
  }
  return true;
}

/**
* 文字列を追加する.
*
* @param position 表示開始座標.
* @param str      追加する文字列.
*
* @retval true  追加成功.
* @retval false 追加失敗.
*/
bool Renderer::AddString(const glm::vec2& position, const char* str)
{
  if (!progFont) {
    return false;
  }
  std::wstring tmp;
  tmp.resize(strlen(str) + 1);
  mbstowcs(&tmp[0], str, tmp.size());
  return AddString(position, tmp.c_str());
}

/**
* 文字列を追加する.
*
* @param position 表示開始座標.
* @param str      追加する文字列.
*
* @retval true  追加成功.
* @retval false 追加失敗.
*/
bool Renderer::AddString(const glm::vec2& position, const wchar_t* str)
{
  if (!progFont) {
    return false;
  }

  const glm::u16vec2 thicknessAndOutline = glm::vec2(0.625f + thickness * 0.375f, border) * 65535.0f;
  Vertex* p = pVBO + vboSize;
  glm::vec2 pos = position * reciprocalScreenSize;
  for (const wchar_t* itr = str; *itr; ++itr) {
    if (vboSize + 4 > vboCapacity) {
      break;
    }
    const FontInfo& font = fontList[*itr];
    if (font.id >= 0 && font.size.x && font.size.y) {
      const glm::vec2 size = font.size * reciprocalScreenSize * scale;
      glm::vec3 offsetedPos = glm::vec3(pos + (font.offset * reciprocalScreenSize) * scale, font.page);
      if (!propotional) {
        offsetedPos.x = pos.x;
      }
      p[0].position = offsetedPos + glm::vec3(0, -size.y, 0);
      p[0].uv = font.uv[0];
      p[0].color = color;
      p[0].subColor = subColor;
      p[0].thicknessAndOutline = thicknessAndOutline;

      p[1].position = offsetedPos + glm::vec3(size.x, -size.y, 0);
      p[1].uv = glm::u16vec2(font.uv[1].x, font.uv[0].y);
      p[1].color = color;
      p[1].subColor = subColor;
      p[1].thicknessAndOutline = thicknessAndOutline;

      p[2].position = offsetedPos + glm::vec3(size.x, 0, 0);
      p[2].uv = font.uv[1];
      p[2].color = color;
      p[2].subColor = subColor;
      p[2].thicknessAndOutline = thicknessAndOutline;

      p[3].position = offsetedPos;
      p[3].uv = glm::u16vec2(font.uv[0].x, font.uv[1].y);
      p[3].color = color;
      p[3].subColor = subColor;
      p[3].thicknessAndOutline = thicknessAndOutline;

      p += 4;
      vboSize += 4;
    }
    float advance;
    if (propotional) {
      advance = font.xadvance;
    } else {
      advance = fixedAdvance;
      if (font.id < 128) {
        advance *= 0.5f;
      }
    }
    pos.x += advance * reciprocalScreenSize.x * scale.x;
  }
  return true;
}

/**
* 文字色を設定する.
*
* @param c 文字色.
*/
void Renderer::Color(const glm::vec4& c)
{
  color = glm::clamp(c, 0.0f, 1.0f) * 255.0f;
}

/**
* 文字色取得する.
*
* @return 文字色.
*/
glm::vec4 Renderer::Color() const
{
  return glm::vec4(color) * (1.0f / 255.0f);
}

/**
* サブ文字色を設定する.
*
* @param c 文字色.
*/
void Renderer::SubColor(const glm::vec4& c)
{
  subColor = glm::clamp(c, 0.0f, 1.0f) * 255.0f;
}

/**
* サブ文字色取得する.
*
* @return 文字色.
*/
glm::vec4 Renderer::SubColor() const
{
  return glm::vec4(subColor) * (1.0f / 255.0f);
}

/**
* VBOをシステムメモリにマッピングする.
*/
void Renderer::BeginUpdate()
{
  if (pVBO) {
    return;
  }
  glBindBuffer(GL_ARRAY_BUFFER, vbo.Id());
  pVBO = static_cast<Vertex*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * vboCapacity, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
  if (!pVBO) {
    const GLenum err = glGetError();
    std::cerr << "ERROR: MapBuffer失敗(0x" << std::hex << err << ")" << std::endl;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  vboSize = 0;
}

/**
* VBOのマッピングを解除する.
*/
void Renderer::EndUpdate()
{
  if (!pVBO || vboSize == 0) {
    return;
  }
  glBindBuffer(GL_ARRAY_BUFFER, vbo.Id());
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  pVBO = nullptr;
}

/**
* フォントを描画する.
*/
void Renderer::Draw() const
{
  if (vboSize > 0) {
    vao.Bind();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    progFont->UseProgram();
    for (size_t i = 0; i < texList.size(); ++i) {
      progFont->BindTexture(GL_TEXTURE0 + i, GL_TEXTURE_2D, texList[i]->Id());
    }
    glDrawElements(GL_TRIANGLES, (vboSize / 4) * 6, GL_UNSIGNED_SHORT, 0);
    vao.Unbind();
  }
}

} // namespace Font