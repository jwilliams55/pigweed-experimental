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

#include "pw_display/display.h"

#include <cinttypes>

namespace pw::display {

namespace {

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

}  // namespace

void Init() {}

int GetWidth() { return kDisplayWidth; }
int GetHeight() { return kDisplayHeight; }

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {}

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {}

bool TouchscreenAvailable() { return false; }

bool NewTouchEvent() { return false; }

pw::coordinates::Vec3Int GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  return point;
}

Status InitFramebuffer(FramebufferRgb565* framebuffer) { return OkStatus(); }

}  // namespace pw::display
