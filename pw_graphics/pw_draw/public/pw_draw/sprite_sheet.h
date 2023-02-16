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

#include "pw_color/color.h"
#include "pw_framebuffer/framebuffer.h"

namespace pw::draw {

class SpriteSheet {
 public:
  const int width;
  const int height;
  const int count;
  const pw::color::color_rgb565_t transparent_color;
  const pw::color::color_rgb565_t* _data;

  int current_index = 0;
  int index_direction = 1;

  pw::color::color_rgb565_t GetColor(int x, int y, int sprite_index);
  void SetIndex(int index);
  void RotateIndexLoop();
  void RotateIndexPingPong();
};

}  // namespace pw::draw
