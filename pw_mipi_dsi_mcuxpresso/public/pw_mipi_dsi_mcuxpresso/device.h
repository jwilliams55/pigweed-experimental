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

#include "fsl_dc_fb_dsi_cmd.h"
#include "fsl_mipi_dsi.h"
#include "fsl_mipi_dsi_smartdma.h"
#include "fsl_rm67162.h"
#include "pw_color/color.h"
#include "pw_framebuffer_pool/framebuffer_pool.h"
#include "pw_math/size.h"
#include "pw_mipi_dsi/device.h"
#include "pw_mipi_dsi_mcuxpresso/framebuffer_device.h"
#include "pw_status/status.h"

namespace pw::mipi::dsi {

constexpr size_t kBytesPerPixel = sizeof(pw::color::color_rgb565_t);
constexpr uint32_t kMaxDSITxArraySize =
    (((FSL_DSI_TX_MAX_PAYLOAD_BYTE)-1U) / kBytesPerPixel) * kBytesPerPixel;

#if !defined(USE_DSI_SMARTDMA)
#error "USE_DSI_SMARTDMA not defined"
#endif

// MIPI DSI Device implementation for the MCUXpresso platform.
class MCUXpressoDevice : public Device {
 public:
  MCUXpressoDevice(
      const pw::framebuffer_pool::FramebufferPool& framebuffer_pool,
      const pw::math::Size<uint16_t>& panel_size,
      video_pixel_format_t pixel_format);
  virtual ~MCUXpressoDevice();

  Status Init();

  // Retrieve a framebuffer for use. Will block until a framebuffer is
  // available.
  pw::framebuffer::Framebuffer GetFramebuffer();

  // pw::mipi::dsi::Device implementation:
  void WriteFramebuffer(pw::framebuffer::Framebuffer framebuffer,
                        WriteCallback write_callback) override;

 public:
  static status_t DSI_Transfer(dsi_transfer_t* xfer);
  static status_t DSI_MemWrite(uint8_t virtualChannel,
                               const uint8_t* data,
                               uint32_t length);
  static void PullPanelResetPin(bool pullUp);
  static void PullPanelPowerPin(bool pullUp);
  void DisplayTEPinHandler();
  Status PrepareDisplayController();

 private:
  struct DSIMemWriteContext {
    volatile bool ongoing;
    const uint8_t* tx_data;
    uint32_t num_bytes_remaining;
    uint8_t dsc_cmd;
  };

  static void DsiSmartDMAMemWriteCallback(MIPI_DSI_HOST_Type* base,
                                          dsi_smartdma_handle_t* handle,
                                          status_t status,
                                          void* userData);
  static void DsiMemWriteCallback(MIPI_DSI_HOST_Type* base,
                                  dsi_handle_t* handle,
                                  status_t status,
                                  void* userData);
  status_t DsiMemWriteSendChunck();
  Status InitDisplayInterface();
  Status InitLcdPanel();
  void InitMipiPanelTEPin();
  void InitMipiDsiClock();
  void SetMipiDsiConfig();
#if USE_DSI_SMARTDMA
  void InitSmartDMA();
#endif

  const pw::framebuffer_pool::FramebufferPool& framebuffer_pool_;
  FramebufferDevice fbdev_;
  dsi_smartdma_handle_t dsi_smartdma_driver_handle_ = {};
  DSIMemWriteContext dsi_mem_write_ctx_ = {};
  dsi_transfer_t dsi_mem_write_xfer_ = {};
  dsi_handle_t dsi_driver_handle_ = {};
  uint8_t dsi_mem_write_tmp_array_[kMaxDSITxArraySize];
  uint32_t mipi_dsi_tx_esc_clk_freq_hz_ = 0;
  uint32_t mipi_dsi_dphy_bit_clk_freq_hz_ = 0;
  mipi_dsi_device_t dsi_device_;
  rm67162_resource_t rm67162_resource_;
  display_handle_t display_handle_;
  dc_fb_dsi_cmd_handle_t dc_fb_dsi_cmd_handle_;
  const dc_fb_dsi_cmd_config_t panel_config_;
  dc_fb_t dc_;
};

}  // namespace pw::mipi::dsi
