// Copyright 2022 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

namespace pw::coordinates {

struct Vec3Int {
  Vec3Int(int new_x, int new_y, int new_z) : x(new_x), y(new_y), z(new_z) {}
  Vec3Int() : x(0), y(0), z(0) {}
  int x;
  int y;
  int z;
};

template <typename T>
struct Vector2 {
  T x;
  T y;
};

template <typename T>
struct Size {
  T width;
  T height;

  bool operator!=(const Size<T>& rhs) const {
    return width != rhs.width || height != rhs.height;
  }
  bool operator==(const Size<T>& rhs) const {
    return width == rhs.width && height == rhs.height;
  }
};

}  // namespace pw::coordinates
