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
#include "app_common/common.h"
#include "board.h"
#include "fsl_iopctl.h"
#include "pin_mux.h"
#include "pw_display_driver_mipi/display_driver.h"
#include "pw_framebuffer_pool_mcuxpresso/framebuffer_pool.h"
#include "pw_mipi_dsi_mcuxpresso/device.h"
#include "pw_status/try.h"

using pw::Status;
using pw::color::color_rgb565_t;
using pw::display::Display;
using pw::display_driver::DisplayDriverMipiDsi;
using pw::framebuffer::PixelFormat;
using pw::framebuffer_pool::FramebufferPoolMCUXpresso;
using pw::mipi::dsi::MCUXpressoDevice;

namespace {

static_assert(DISPLAY_WIDTH > 0);
static_assert(DISPLAY_HEIGHT > 0);

// Framebuffer addresses in on-board PSRAM.
constexpr uint32_t kBuffer0Addr = 0x28000000U;
constexpr uint32_t kBuffer1Addr = 0x28200000U;
constexpr video_pixel_format_t kVideoPixelFormat = kVIDEO_PixelFormatRGB565;
constexpr pw::math::Size<uint16_t> kFramebufferDimensions = {
    .width = FRAMEBUFFER_WIDTH >= 0 ? FRAMEBUFFER_WIDTH : DISPLAY_WIDTH,
    .height = DISPLAY_HEIGHT,
};
constexpr uint16_t kBufferStrideBytes =
    kFramebufferDimensions.width * pw::mipi::dsi::kBytesPerPixel;
constexpr pw::math::Size<uint16_t> kDisplaySize = {DISPLAY_WIDTH,
                                                   DISPLAY_HEIGHT};
const pw::Vector<void*, 2> s_framebuffer_addrs = {
    reinterpret_cast<void*>(kBuffer0Addr),
    reinterpret_cast<void*>(kBuffer1Addr)};

FramebufferPoolMCUXpresso s_fb_pool({
    .fb_addr = s_framebuffer_addrs,
    .dimensions = kFramebufferDimensions,
    .row_bytes = kBufferStrideBytes,
    .pixel_format = PixelFormat::RGB565,
});
MCUXpressoDevice s_mipi_device(s_fb_pool, kDisplaySize, kVideoPixelFormat);
DisplayDriverMipiDsi s_display_driver(s_mipi_device, kDisplaySize);
Display s_display(s_display_driver, kDisplaySize, s_fb_pool);

void InitMipiPins(void) {
  constexpr uint32_t kPwmModeFunc =
      (IOPCTL_PIO_FUNC0 | IOPCTL_PIO_PUPD_DI | IOPCTL_PIO_PULLDOWN_EN |
       IOPCTL_PIO_INBUF_EN | IOPCTL_PIO_SLEW_RATE_NORMAL |
       IOPCTL_PIO_FULLDRIVE_DI | IOPCTL_PIO_ANAMUX_DI | IOPCTL_PIO_PSEDRAIN_DI |
       IOPCTL_PIO_INV_DI);
  IOPCTL_PinMuxSet(IOPCTL, BOARD_MIPI_BL_PORT, BOARD_MIPI_BL_PIN, kPwmModeFunc);

  constexpr uint32_t kPwrEnModeFunc =
      (IOPCTL_PIO_FUNC0 | IOPCTL_PIO_PUPD_DI | IOPCTL_PIO_PULLDOWN_EN |
       IOPCTL_PIO_INBUF_EN | IOPCTL_PIO_SLEW_RATE_NORMAL |
       IOPCTL_PIO_FULLDRIVE_DI | IOPCTL_PIO_ANAMUX_DI | IOPCTL_PIO_PSEDRAIN_DI |
       IOPCTL_PIO_INV_DI);
  IOPCTL_PinMuxSet(
      IOPCTL, BOARD_MIPI_POWER_PORT, BOARD_MIPI_POWER_PIN, kPwrEnModeFunc);

  constexpr uint32_t kPort2Pin18ModeFunc =
      (IOPCTL_PIO_FUNC0 | IOPCTL_PIO_PUPD_EN | IOPCTL_PIO_PULLDOWN_EN |
       IOPCTL_PIO_INBUF_EN | IOPCTL_PIO_SLEW_RATE_NORMAL |
       IOPCTL_PIO_FULLDRIVE_DI | IOPCTL_PIO_ANAMUX_DI | IOPCTL_PIO_PSEDRAIN_DI |
       IOPCTL_PIO_INV_DI);
  IOPCTL_PinMuxSet(IOPCTL, 3U, 18U, kPort2Pin18ModeFunc);

  constexpr uint32_t kResetBModeFunc =
      (IOPCTL_PIO_FUNC0 | IOPCTL_PIO_PUPD_DI | IOPCTL_PIO_PULLDOWN_EN |
       IOPCTL_PIO_INBUF_EN | IOPCTL_PIO_SLEW_RATE_NORMAL |
       IOPCTL_PIO_FULLDRIVE_DI | IOPCTL_PIO_ANAMUX_DI | IOPCTL_PIO_PSEDRAIN_DI |
       IOPCTL_PIO_INV_DI);
  IOPCTL_PinMuxSet(
      IOPCTL, BOARD_MIPI_RST_PORT, BOARD_MIPI_RST_PIN, kResetBModeFunc);
}

}  // namespace

// static
Status Common::Init() {
  InitMipiPins();
  BOARD_InitPsRam();

  GPIO_PortInit(GPIO, BOARD_MIPI_POWER_PORT);
  GPIO_PortInit(GPIO, BOARD_MIPI_BL_PORT);
  GPIO_PortInit(GPIO, BOARD_MIPI_RST_PORT);
  GPIO_PortInit(GPIO, BOARD_MIPI_TE_PORT);

  BOARD_BootClockRUN();

  PW_TRY(s_fb_pool.Init(&s_mipi_device));
  PW_TRY(s_mipi_device.Init());
  return s_display_driver.Init();
}

// static
pw::display::Display& Common::GetDisplay() { return s_display; }
