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

#include "pw_coordinates/vec_int.h"
#include "pw_framebuffer/rgb565.h"
#include "pw_status/status.h"

using namespace pw::framebuffer;

namespace pw::display {

void Init();
// TODO(tonymd): Add a DPI or physical size value.
int GetWidth();
int GetHeight();
// TODO(tonymd): Add update functions for new framebuffer types or make this a
// templated class.
void Update(FramebufferRgb565* framebuffer);
// Initialize the supplied |framebuffer| to the appropriate size for the
// display.
Status InitFramebuffer(FramebufferRgb565* framebuffer);
void UpdatePixelDouble(FramebufferRgb565* framebuffer);
bool TouchscreenAvailable();
bool NewTouchEvent();
pw::coordinates::Vec3Int GetTouchPoint();

}  // namespace pw::display
