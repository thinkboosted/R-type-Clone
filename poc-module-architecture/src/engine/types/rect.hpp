
#pragma once

#include "vector.hpp"

namespace rtypeEngine {
struct Rectf2d {
  Vector2f position;
  Vector2f size;
};

struct Recti2d {
  Vector2i position;
  Vector2i size;
};

struct Rectu2d {
  Vector2u position;
  Vector2u size;
};

struct Rectf3d {
  Vector3f position;
  Vector3f size;
};

struct Recti3d {
  Vector3i position;
  Vector3i size;
};

struct Rectu3d {
  Vector3u position;
  Vector3u size;
};
}  // namespace rtypeEngine