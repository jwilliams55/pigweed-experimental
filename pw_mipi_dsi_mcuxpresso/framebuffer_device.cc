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

#include "pw_mipi_dsi_mcuxpresso/framebuffer_device.h"

#include "common.h"
#include "pw_assert/assert.h"
#include "pw_status/try.h"

namespace pw::mipi::dsi {

FramebufferDevice::FramebufferDevice(uint8_t layer)
    : dc_(nullptr), layer_(layer), enabled_(false) {
  VIDEO_MEMPOOL_InitEmpty(&video_mempool_);
}

Status FramebufferDevice::Init(
    const dc_fb_t* dc, const pw::framebuffer::pool::PoolData& pool_data) {
  PW_TRY(InitDisplayController(dc));
  return InitVideoMemPool(pool_data);
}

Status FramebufferDevice::InitDisplayController(const dc_fb_t* dc) {
  PW_ASSERT(!dc_);
  dc_ = dc;

  status_t status = dc_->ops->init(dc);
  if (status != kStatus_Success) {
    return MCUXpressoToPigweedStatus(status);
  }

  dc_fb_info_t buff_info;

  status = dc_->ops->getLayerDefaultConfig(dc_, layer_, &buff_info);
  if (status != kStatus_Success) {
    return MCUXpressoToPigweedStatus(status);
  }

  dc_->ops->setCallback(dc_, layer_, BufferSwitchOffCallback, this);

  status = dc_->ops->setLayerConfig(dc_, layer_, &buff_info);
  if (status != kStatus_Success) {
    return MCUXpressoToPigweedStatus(status);
  }

  return OkStatus();
}

Status FramebufferDevice::InitVideoMemPool(
    const pw::framebuffer::pool::PoolData& pool_data) {
  if (enabled_) {
    return Status::FailedPrecondition();
  }

  for (uint8_t i = 0; i < pool_data.num_fb; i++) {
    VIDEO_MEMPOOL_Put(&video_mempool_, pool_data.fb_addr[i]);
  }

  return OkStatus();
}

Status FramebufferDevice::Close() {
  if (!dc_)
    return Status::FailedPrecondition();

  status_t status = dc_->ops->deinit(dc_);
  dc_ = nullptr;
  return MCUXpressoToPigweedStatus(status);
}

Status FramebufferDevice::Enable() {
  if (enabled_)
    return OkStatus();

  status_t status = kStatus_Success;

  if ((dc_->ops->getProperty(dc_) & (uint32_t)kDC_FB_ReserveFrameBuffer) ==
      0U) {
    status = dc_->ops->enableLayer(dc_, layer_);
    if (status != kStatus_Success) {
      enabled_ = true;
    }
  }

  return MCUXpressoToPigweedStatus(status);
}

Status FramebufferDevice::Disable() {
  if (!enabled_)
    return OkStatus();

  status_t status = dc_->ops->disableLayer(dc_, layer_);
  enabled_ = false;

  return MCUXpressoToPigweedStatus(status);
}

Status FramebufferDevice::WriteFramebuffer(void* frameBuffer) {
  return MCUXpressoToPigweedStatus(
      dc_->ops->setFrameBuffer(dc_, layer_, frameBuffer));
}

void* FramebufferDevice::GetFramebuffer() {
  return VIDEO_MEMPOOL_Get(&video_mempool_);
}

void FramebufferDevice::BufferSwitchOff(void* buffer) {
  VIDEO_MEMPOOL_Put(&video_mempool_, buffer);
}

// static
void FramebufferDevice::BufferSwitchOffCallback(void* param, void* buffer) {
  PW_ASSERT(param != nullptr);
  static_cast<FramebufferDevice*>(param)->BufferSwitchOff(buffer);
}

}  // namespace pw::mipi::dsi
