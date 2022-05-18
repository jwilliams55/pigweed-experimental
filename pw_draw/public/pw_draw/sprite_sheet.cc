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
#include "pw_draw/sprite_sheet.h"

namespace pw::draw {

color_rgb565_t SpriteSheet::GetColor(int x, int y, int sprite_index = 0) {
  int start_y = sprite_index * height + y;
  return _data[start_y * width + x];
}

void SpriteSheet::SetIndex(int index) { current_index = index; }

void SpriteSheet::RotateIndexLoop() {
  current_index = (current_index + 1) % count;
}

void SpriteSheet::RotateIndexPingPong() {
  current_index += index_direction;
  if (current_index == 0 || current_index == count - 1) {
    index_direction *= -1;
  }
}

}  // namespace pw::draw
