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

namespace pw::display {

Display::Display(pw::framebuffer::FramebufferRgb565 framebuffer,
                 pw::display_driver::DisplayDriver& display_driver)
    : framebuffer_(std::move(framebuffer)), display_driver_(display_driver) {}

Display::~Display() = default;

Status Display::InitFramebuffer(
    pw::framebuffer::FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_.GetFramebufferData(),
                                  framebuffer_.GetWidth(),
                                  framebuffer_.GetHeight(),
                                  framebuffer_.GetRowBytes());
  return OkStatus();
}

void Display::Update(pw::framebuffer::FramebufferRgb565& framebuffer) {
  display_driver_.Update(&framebuffer).IgnoreError();
}

}  // namespace pw::display
