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

#include "pw_mipi_dsi_mcuxpresso/device.h"

#include <cstring>

#include "board.h"
#include "common.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"
#include "fsl_mipi_dsi.h"
#include "fsl_mipi_dsi_smartdma.h"
#include "fsl_power.h"
#include "pin_mux.h"

using pw::color::color_rgb565_t;
using pw::framebuffer::FramebufferRgb565;

namespace pw::mipi::dsi {

namespace {

constexpr uint8_t kMipiDsiLaneNum = 1;
constexpr IRQn_Type kMipiDsiIrqn = MIPI_IRQn;
constexpr int kVideoLayer = 0;

// This class is currently a singleton because the some callbacks and IRQ
// handlers do not have a user-data param.
MCUXpressoDevice* s_device;

extern "C" {
void GPIO_INTA_DriverIRQHandler(void) {
  uint32_t intStat = GPIO_PortGetInterruptStatus(GPIO, BOARD_MIPI_TE_PORT, 0);

  GPIO_PortClearInterruptFlags(GPIO, BOARD_MIPI_TE_PORT, 0, intStat);

  if (s_device && intStat & (1U << BOARD_MIPI_TE_PIN)) {
    s_device->DisplayTEPinHandler();
  }
}

void SDMA_DriverIRQHandler(void) { SMARTDMA_HandleIRQ(); }
}  // extern "C"

}  // namespace

MCUXpressoDevice::MCUXpressoDevice(
    const pw::framebuffer::pool::PoolData& fb_pool,
    const pw::coordinates::Size<uint16_t>& panel_size,
    video_pixel_format_t pixel_format)
    : fb_pool_(fb_pool),
      fbdev_(kVideoLayer),
      dsi_device_({
          .virtualChannel = 0,
          .xferFunc = MCUXpressoDevice::DSI_Transfer,
          .memWriteFunc = MCUXpressoDevice::DSI_MemWrite,
          .callback = nullptr,
          .userData = nullptr,
      }),
      rm67162_resource_({
          .dsiDevice = &dsi_device_,
          .pullResetPin = MCUXpressoDevice::PullPanelResetPin,
          .pullPowerPin = MCUXpressoDevice::PullPanelPowerPin,
      }),
      display_handle_({
          .resource = &rm67162_resource_,
          .ops = &rm67162_ops,
          .width = panel_size.width,
          .height = panel_size.height,
          .pixelFormat = pixel_format,
      }),
      dc_fb_dsi_cmd_handle_({
          .dsiDevice = &dsi_device_,
          .panelHandle = &display_handle_,
          .initTimes = 0,
          .enabledLayerCount = 0,
          .layers = {},
          .useTEPin = true,
      }),
      panel_config_({
          .commonConfig =
              {
                  .resolution =
                      FSL_VIDEO_RESOLUTION(panel_size.width, panel_size.height),
                  .hsw = 0,  // Unused.
                  .hfp = 0,  // Unused.
                  .hbp = 0,  // Unused.
                  .vsw = 0,  // Unused.
                  .vfp = 0,  // Unused.
                  .vbp = 0,  // Unused.
                  .controlFlags = 0,
                  .dsiLanes = kMipiDsiLaneNum,
                  .pixelClock_Hz = 0,  // Unsure of correct value.
                  .pixelFormat = pixel_format,
              },
          .useTEPin = true,
      }),
      dc_({
          .ops = &g_dcFbOpsDsiCmd,
          .prvData = &dc_fb_dsi_cmd_handle_,
          .config = &panel_config_,
      }) {
  PW_ASSERT(s_device == nullptr);
  s_device = this;
}

MCUXpressoDevice::~MCUXpressoDevice() = default;

Status MCUXpressoDevice::Init() {
  if (fb_pool_.num_fb == 0)
    return Status::InvalidArgument();

  Status s = PrepareDisplayController();
  if (!s.ok())
    return s;

  s = fbdev_.Init(&dc_, fb_pool_);
  if (!s.ok())
    return s;

  // Clear buffer to black - it is shown once screen is enabled.
  void* buffer = fbdev_.GetFramebuffer();
  if (!buffer) {
    return Status::Internal();
  }
  std::memset(buffer, 0, fb_pool_.row_bytes * fb_pool_.size.height);
  s = fbdev_.WriteFramebuffer(buffer);
  if (!s.ok())
    return s;

  return fbdev_.Enable();
}

FramebufferRgb565 MCUXpressoDevice::GetFramebuffer() {
  return FramebufferRgb565(
      static_cast<color_rgb565_t*>(fbdev_.GetFramebuffer()),
      fb_pool_.size.width,
      fb_pool_.size.height,
      fb_pool_.row_bytes);
}

Status MCUXpressoDevice::ReleaseFramebuffer(FramebufferRgb565 framebuffer) {
  if (!framebuffer.IsValid())
    return Status::InvalidArgument();
  void* data = framebuffer.GetFramebufferData();
  return fbdev_.WriteFramebuffer(data);
}

Status MCUXpressoDevice::PrepareDisplayController(void) {
  Status status = InitDisplayInterface();
  if (!status.ok())
    return status;

#if USE_DSI_SMARTDMA
  InitSmartDMA();
  status_t s = DSI_TransferCreateHandleSMARTDMA(
      MIPI_DSI_HOST,
      &dsi_smartdma_driver_handle_,
      MCUXpressoDevice::DsiSmartDMAMemWriteCallback,
      this);

  return MCUXpressoToPigweedStatus(s);

#else

  NVIC_SetPriority(kMipiDsiIrqn, 6);

  memset(&dsi_mem_write_ctx_, 0, sizeof(DSIMemWriteContext));

  return MCUXpressoToPigweedStatus(
      DSI_TransferCreateHandle(MIPI_DSI_HOST,
                               &dsi_driver_handle_,
                               MCUXpressoDevice::DsiMemWriteCallback,
                               this));
#endif
}

// static
Status MCUXpressoDevice::InitDisplayInterface() {
  RESET_SetPeripheralReset(kMIPI_DSI_PHY_RST_SHIFT_RSTn);
  InitMipiDsiClock();
  RESET_ClearPeripheralReset(kMIPI_DSI_CTRL_RST_SHIFT_RSTn);
  SetMipiDsiConfig();
  RESET_ClearPeripheralReset(kMIPI_DSI_PHY_RST_SHIFT_RSTn);
  return InitLcdPanel();
}

Status MCUXpressoDevice::InitLcdPanel() {
  const gpio_pin_config_t pinConfig = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0,
  };

  GPIO_PinInit(GPIO, BOARD_MIPI_POWER_PORT, BOARD_MIPI_POWER_PIN, &pinConfig);
  GPIO_PinInit(GPIO, BOARD_MIPI_RST_PORT, BOARD_MIPI_RST_PIN, &pinConfig);

  InitMipiPanelTEPin();

  return OkStatus();
}

// static
void MCUXpressoDevice::InitMipiDsiClock(void) {
  POWER_DisablePD(kPDRUNCFG_APD_MIPIDSI_SRAM);
  POWER_DisablePD(kPDRUNCFG_PPD_MIPIDSI_SRAM);
  POWER_DisablePD(kPDRUNCFG_PD_MIPIDSI);
  POWER_ApplyPD();

  CLOCK_AttachClk(kFRO_DIV1_to_MIPI_DPHYESC_CLK);
  CLOCK_SetClkDiv(kCLOCK_DivDphyEscRxClk, 4);
  CLOCK_SetClkDiv(kCLOCK_DivDphyEscTxClk, 3);
  mipi_dsi_tx_esc_clk_freq_hz_ = CLOCK_GetMipiDphyEscTxClkFreq();

  CLOCK_AttachClk(kAUX1_PLL_to_MIPI_DPHY_CLK);
#if (DEMO_RM67162_BUFFER_FORMAT == PIXEL_FORMAT_RGB565)
  CLOCK_InitSysPfd(kCLOCK_Pfd3, 30);
#else
  CLOCK_InitSysPfd(kCLOCK_Pfd3, 19);
#endif
  CLOCK_SetClkDiv(kCLOCK_DivDphyClk, 1);
  mipi_dsi_dphy_bit_clk_freq_hz_ = CLOCK_GetMipiDphyClkFreq();
}

// static
void MCUXpressoDevice::InitMipiPanelTEPin(void) {
  const gpio_pin_config_t tePinConfig = {
      .pinDirection = kGPIO_DigitalInput,
      .outputLogic = 0,
  };

  gpio_interrupt_config_t te_pin_int_config = {kGPIO_PinIntEnableEdge,
                                               kGPIO_PinIntEnableHighOrRise};

  GPIO_PinInit(GPIO, BOARD_MIPI_TE_PORT, BOARD_MIPI_TE_PIN, &tePinConfig);

  GPIO_SetPinInterruptConfig(
      GPIO, BOARD_MIPI_TE_PORT, BOARD_MIPI_TE_PIN, &te_pin_int_config);

  GPIO_PinEnableInterrupt(GPIO, BOARD_MIPI_TE_PORT, BOARD_MIPI_TE_PIN, 0);

  NVIC_SetPriority(GPIO_INTA_IRQn, 3);

  NVIC_EnableIRQ(GPIO_INTA_IRQn);
}

// static
void MCUXpressoDevice::SetMipiDsiConfig() {
  dsi_config_t dsiConfig;
  dsi_dphy_config_t dphyConfig;

  DSI_GetDefaultConfig(&dsiConfig);
  dsiConfig.numLanes = kMipiDsiLaneNum;
  dsiConfig.autoInsertEoTp = true;

  DSI_GetDphyDefaultConfig(&dphyConfig,
                           mipi_dsi_dphy_bit_clk_freq_hz_,
                           mipi_dsi_tx_esc_clk_freq_hz_);

  DSI_Init(MIPI_DSI_HOST, &dsiConfig);

  DSI_InitDphy(MIPI_DSI_HOST, &dphyConfig, 0);
}

#if USE_DSI_SMARTDMA
// static
void MCUXpressoDevice::InitSmartDMA() {
  RESET_ClearPeripheralReset(kINPUTMUX_RST_SHIFT_RSTn);

  INPUTMUX_Init(INPUTMUX);
  INPUTMUX_AttachSignal(INPUTMUX, 0, kINPUTMUX_MipiIrqToSmartDmaInput);

  INPUTMUX_Deinit(INPUTMUX);

  POWER_DisablePD(kPDRUNCFG_APD_SMARTDMA_SRAM);
  POWER_DisablePD(kPDRUNCFG_PPD_SMARTDMA_SRAM);
  POWER_ApplyPD();

  RESET_ClearPeripheralReset(kSMART_DMA_RST_SHIFT_RSTn);
  CLOCK_EnableClock(kCLOCK_Smartdma);

  SMARTDMA_InitWithoutFirmware();
  NVIC_EnableIRQ(SDMA_IRQn);

  NVIC_SetPriority(SDMA_IRQn, 3);
}
#endif

// static
status_t MCUXpressoDevice::DSI_Transfer(dsi_transfer_t* xfer) {
  return DSI_TransferBlocking(MIPI_DSI_HOST, xfer);
}

// static
status_t MCUXpressoDevice::DSI_MemWrite(uint8_t virtualChannel,
                                        const uint8_t* data,
                                        uint32_t length) {
#if USE_DSI_SMARTDMA
  dsi_smartdma_write_mem_transfer_t xfer = {
#if (DEMO_RM67162_BUFFER_FORMAT == PIXEL_FORMAT_RGB565)
    .inputFormat = kDSI_SMARTDMA_InputPixelFormatRGB565,
    .outputFormat = kDSI_SMARTDMA_OutputPixelFormatRGB565,
#elif (DEMO_RM67162_BUFFER_FORMAT == PIXEL_FORMAT_RGB888)
    .inputFormat = kDSI_SMARTDMA_InputPixelFormatRGB888,
    .outputFormat = kDSI_SMARTDMA_OutputPixelFormatRGB888,
#else
    .inputFormat = kDSI_SMARTDMA_InputPixelFormatXRGB8888,
    .outputFormat = kDSI_SMARTDMA_OutputPixelFormatRGB888,
#endif /* DEMO_RM67162_BUFFER_FORMAT */
    .data = data,
    .dataSize = length,

    .virtualChannel = virtualChannel,
    .disablePixelByteSwap = false,
  };

  return DSI_TransferWriteMemorySMARTDMA(
      MIPI_DSI_HOST, &s_device->dsi_smartdma_driver_handle_, &xfer);

#else /* USE_DSI_SMARTDMA */

  status_t status;

  if (s_device->dsi_mem_write_ctx_.ongoing) {
    return kStatus_Fail;
  }

  s_device->dsi_mem_write_xfer_.virtualChannel = virtualChannel;
  s_device->dsi_mem_write_xfer_.flags = kDSI_TransferUseHighSpeed;
  s_device->dsi_mem_write_xfer_.sendDscCmd = true;

  s_device->dsi_mem_write_ctx_.ongoing = true;
  s_device->dsi_mem_write_ctx_.tx_data = data;
  s_device->dsi_mem_write_ctx_.num_bytes_remaining = length;
  s_device->dsi_mem_write_ctx_.dsc_cmd = kMIPI_DCS_WriteMemoryStart;

  status = s_device->DsiMemWriteSendChunck();

  if (status != kStatus_Success) {
    /* Memory write does not start actually. */
    s_device->dsi_mem_write_ctx_.ongoing = false;
  }

  return status;
#endif
}

// static
void MCUXpressoDevice::PullPanelResetPin(bool pullUp) {
  if (pullUp) {
    GPIO_PinWrite(GPIO, BOARD_MIPI_RST_PORT, BOARD_MIPI_RST_PIN, 1);
  } else {
    GPIO_PinWrite(GPIO, BOARD_MIPI_RST_PORT, BOARD_MIPI_RST_PIN, 0);
  }
}

// static
void MCUXpressoDevice::PullPanelPowerPin(bool pullUp) {
  if (pullUp) {
    GPIO_PinWrite(GPIO, BOARD_MIPI_POWER_PORT, BOARD_MIPI_POWER_PIN, 1);
  } else {
    GPIO_PinWrite(GPIO, BOARD_MIPI_POWER_PORT, BOARD_MIPI_POWER_PIN, 0);
  }
}

// static
void MCUXpressoDevice::DisplayTEPinHandler() {
  DC_FB_DSI_CMD_TE_IRQHandler(&dc_);
}

// static
status_t MCUXpressoDevice::DsiMemWriteSendChunck(void) {
  uint32_t curSendLen;
  uint32_t i;

  curSendLen = kMaxDSITxArraySize > dsi_mem_write_ctx_.num_bytes_remaining
                   ? dsi_mem_write_ctx_.num_bytes_remaining
                   : kMaxDSITxArraySize;

  dsi_mem_write_xfer_.txDataType = kDSI_TxDataDcsLongWr;
  dsi_mem_write_xfer_.dscCmd = dsi_mem_write_ctx_.dsc_cmd;
  dsi_mem_write_xfer_.txData = dsi_mem_write_tmp_array_;
  dsi_mem_write_xfer_.txDataSize = curSendLen;

#if (DEMO_RM67162_BUFFER_FORMAT == PIXEL_FORMAT_RGB565)
  for (i = 0; i < curSendLen; i += 2) {
    dsi_mem_write_tmp_array_[i] = *(dsi_mem_write_ctx_.tx_data + 1);
    dsi_mem_write_tmp_array_[i + 1] = *(dsi_mem_write_ctx_.tx_data);

    dsi_mem_write_ctx_.tx_data += 2;
  }
#else
  for (i = 0; i < curSendLen; i += 3) {
    dsi_mem_write_tmp_array_[i] = *(dsi_mem_write_ctx_.tx_data + 2);
    dsi_mem_write_tmp_array_[i + 1] = *(dsi_mem_write_ctx_.tx_data + 1);
    dsi_mem_write_tmp_array_[i + 2] = *(dsi_mem_write_ctx_.tx_data);

    dsi_mem_write_ctx_.tx_data += 3;
  }
#endif

  dsi_mem_write_ctx_.num_bytes_remaining -= curSendLen;
  dsi_mem_write_ctx_.dsc_cmd = kMIPI_DCS_WriteMemoryContinue;

  return DSI_TransferNonBlocking(
      MIPI_DSI_HOST, &dsi_driver_handle_, &dsi_mem_write_xfer_);
}

// static
void MCUXpressoDevice::DsiMemWriteCallback(MIPI_DSI_HOST_Type* base,
                                           dsi_handle_t* handle,
                                           status_t status,
                                           void* userData) {
  MCUXpressoDevice* device = static_cast<MCUXpressoDevice*>(userData);
  if ((kStatus_Success == status) &&
      (device->dsi_mem_write_ctx_.num_bytes_remaining > 0)) {
    status = device->DsiMemWriteSendChunck();
    if (kStatus_Success == status) {
      return;
    }
  }

  device->dsi_mem_write_ctx_.ongoing = false;
  MIPI_DSI_MemoryDoneDriverCallback(status, &device->dsi_device_);
}

// static
void MCUXpressoDevice::DsiSmartDMAMemWriteCallback(
    MIPI_DSI_HOST_Type* base,
    dsi_smartdma_handle_t* handle,
    status_t status,
    void* userData) {
  MCUXpressoDevice* device = static_cast<MCUXpressoDevice*>(userData);
  MIPI_DSI_MemoryDoneDriverCallback(status, &device->dsi_device_);
}

}  // namespace pw::mipi::dsi
