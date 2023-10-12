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

#include <GLFW/glfw3.h>

#include "pw_display_driver/display_driver.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_math/vector3.h"

struct ImGuiMousePosition {
  bool left_button_pressed = false;
  int position_x = 0;
  int position_y = 0;
};

namespace pw::display_driver {

class DisplayDriverImgUI : public DisplayDriver {
 public:
  DisplayDriverImgUI();

  GLFWwindow* GetGlfwWindow();
  ImGuiMousePosition GetImGuiMousePosition();

  // pw::display_driver::DisplayDriver implementation:
  Status Init() override;
  void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                        WriteCallback write_callback) override;
  Status WriteRow(span<uint16_t> row_pixels,
                  uint16_t row_idx,
                  uint16_t col_idx) override;
  uint16_t GetWidth() const override;
  uint16_t GetHeight() const override;

 private:
  void RecreateLcdTexture();
  void Render();
};

}  // namespace pw::display_driver
