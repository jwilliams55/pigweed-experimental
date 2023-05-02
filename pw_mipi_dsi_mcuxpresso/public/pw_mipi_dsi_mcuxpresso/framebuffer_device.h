// Copyright 2023 The Pigweed Authors
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

#include <cstdint>

#include "fsl_dc_fb.h"
#include "fsl_video_common.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_function/function.h"
#include "pw_status/status.h"
#include "pw_sync/counting_semaphore.h"

namespace pw::mipi::dsi {

// FramebufferDevice manages a pool of framebuffers and is responsible for
// writing them to the display using NXPs display controller provided by the
// driver.dc-fb-common.MIMXRT595S SDK component. The framebuffer pool is
// managed by the driver.video-common.MIMXRT595S SDK component.
class FramebufferDevice {
 public:
  using WriteCallback = Callback<void(void*, Status)>;

  // Create a default uninitialized instance. Init must be called to fully
  // initialize an instance before it can be used.
  FramebufferDevice(uint8_t layer);

  pw::Status Init(
      const dc_fb_t* dc,
      const pw::framebuffer_pool::FramebufferPool& framebuffer_pool);

  // Close the device.
  pw::Status Close();

  // Enable the device.
  pw::Status Enable();

  // Disable the device.
  pw::Status Disable();

  // Send the framebuffer data to the device.
  void WriteFramebuffer(void* buffer, WriteCallback write_callback);

  // Retrieve an unused framebuffer. *Will block* until a framebuffer becomes
  // available.
  void* GetFramebuffer();

 private:
  static void BufferSwitchOffCallback(void* param, void* buffer);
  void WriteComplete(void*, Status);

  void BufferSwitchOff(void* buffer);
  pw::Status InitDisplayController(const dc_fb_t* dc);
  Status InitVideoMemPool(
      const pw::framebuffer_pool::FramebufferPool& framebuffer_pool);

  video_mempool_t video_mempool_;
  const dc_fb_t* dc_;    // NXP Display controller.
  const uint8_t layer_;  // The video layer to write to.
  bool enabled_;         // Has this instance been initialized.
  pw::sync::CountingSemaphore framebuffer_semaphore_;
};

}  // namespace pw::mipi::dsi
