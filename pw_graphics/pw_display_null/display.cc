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

#include <cinttypes>

#include "pw_display/display_backend.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

namespace {

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;

}  // namespace

Display::Display() = default;

Display::~Display() = default;

Status Display::Init() { return OkStatus(); }

int Display::GetWidth() const { return kDisplayWidth; }

int Display::GetHeight() const { return kDisplayHeight; }

void Display::Update(pw::framebuffer::FramebufferRgb565& frame_buffer) {}

bool Display::TouchscreenAvailable() const { return false; }

bool Display::NewTouchEvent() { return false; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  return pw::coordinates::Vec3Int{0, 0, 0};
}

Status Display::InitFramebuffer(FramebufferRgb565* framebuffer) {
  return OkStatus();
}

}  // namespace pw::display::backend
