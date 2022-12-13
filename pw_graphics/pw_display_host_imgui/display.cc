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

// LCD Facade using imgui running on a host machine.
// Much of this code is from the imgui example:
// https://github.com/ocornut/imgui/tree/master/examples/example_glfw_opengl3
// As well as the wiki page:
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
#include "pw_display/display_backend.h"

using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

int Display::GetWidth() const { return kDisplayWidth; }

int Display::GetHeight() const { return kDisplayHeight; }

Status Display::Init() { return display_driver_.Init(); }

Display::Display() = default;

Display::~Display() = default;

void Display::Update(FramebufferRgb565& frame_buffer) {
  display_driver_.Update(&frame_buffer);
}

bool Display::TouchscreenAvailable() const { return true; }

bool Display::NewTouchEvent() { return display_driver_.NewTouchEvent(); }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
  return display_driver_.GetTouchPoint();
}

Status Display::InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_data_,
                                  kDisplayWidth,
                                  kDisplayHeight,
                                  kDisplayWidth * sizeof(uint16_t));
  return OkStatus();
}

}  // namespace pw::display::backend
