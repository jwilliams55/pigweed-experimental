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

#include "pw_spi_stm32f429i_disc1_stm32cube/initiator.h"

#include <algorithm>

#include "pw_log/log.h"
#include "pw_status/try.h"
#include "stm32cube/stm32cube.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

namespace pw::spi {

namespace {

constexpr uint32_t kTimeout = 10000;

constexpr Status ConvertStatus(HAL_StatusTypeDef status) {
  switch (status) {
    case HAL_OK:
      return OkStatus();
    case HAL_ERROR:
      return Status::Internal();
    case HAL_BUSY:
      return Status::Unavailable();
    case HAL_TIMEOUT:
      return Status::DeadlineExceeded();
  }
  return Status::NotFound();  // Unreachable.
}

uint32_t GetDataSize(BitsPerWord bits_per_word) {
  if (bits_per_word() == 8) {
    return SPI_DATASIZE_8BIT;
  } else if (bits_per_word() == 16) {
    return SPI_DATASIZE_16BIT;
  }
  PW_ASSERT(false);
  return SPI_DATASIZE_8BIT;
}

constexpr uint32_t GetBitOrder(BitOrder bit_order) {
  switch (bit_order) {
    case BitOrder::kLsbFirst:
      return SPI_FIRSTBIT_LSB;
    case BitOrder::kMsbFirst:
      return SPI_FIRSTBIT_MSB;
  }
  PW_ASSERT(false);
  return SPI_FIRSTBIT_MSB;
}

constexpr uint32_t GetPhase(ClockPhase phase) {
  switch (phase) {
    case ClockPhase::kFallingEdge:
      return SPI_PHASE_1EDGE;
    case ClockPhase::kRisingEdge:
      return SPI_PHASE_2EDGE;
  }
  PW_ASSERT(false);
  return SPI_PHASE_1EDGE;
}

constexpr uint32_t GetPolarity(ClockPolarity polarity) {
  switch (polarity) {
    case ClockPolarity::kActiveHigh:
      return SPI_POLARITY_HIGH;
    case ClockPolarity::kActiveLow:
      return SPI_POLARITY_LOW;
  }
  PW_ASSERT(false);
  return SPI_POLARITY_HIGH;
}

}  // namespace

// A collection of instance variables isolated into a separate structure
// so that clients of Stm32CubeInitiator aren't forced to have a compile-time
// dependency on the STM32 header files.
struct Stm32CubeInitiator::PrivateInstanceData {
  bool initialized = false;
  Status init_status;  // The saved LazyInit() status.
  BitsPerWord desired_bits_per_word_;
  SPI_HandleTypeDef spi_handle;

  PrivateInstanceData()
      : desired_bits_per_word_(8),
        spi_handle{.Instance = SPI5,
                   .Init{.Mode = SPI_MODE_MASTER,
                         .Direction = SPI_DIRECTION_2LINES,
                         .DataSize = SPI_DATASIZE_8BIT,
                         .CLKPolarity = SPI_POLARITY_LOW,
                         .CLKPhase = SPI_PHASE_1EDGE,
                         .NSS = SPI_NSS_SOFT,
                         .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
                         .FirstBit = SPI_FIRSTBIT_MSB,
                         .TIMode = SPI_TIMODE_DISABLE,
                         .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
                         .CRCPolynomial = 7}} {}

  Status InitSpi() {
    auto s = HAL_SPI_Init(&spi_handle);
    auto status = ConvertStatus(s);
    PW_LOG_INFO("HAL_SPI_Init =>: %s", status.str());
    return status;
  }
};

Stm32CubeInitiator::Stm32CubeInitiator()
    : instance_data_(new PrivateInstanceData) {}

Stm32CubeInitiator::~Stm32CubeInitiator() { delete instance_data_; }

Status Stm32CubeInitiator::LazyInit() {
  if (instance_data_->initialized)
    return instance_data_->init_status;
  instance_data_->init_status = instance_data_->InitSpi();
  instance_data_->initialized = true;
  PW_LOG_INFO("Stm32CubeInitiator::LazyInit: %s",
              instance_data_->init_status.str());
  return instance_data_->init_status;
}

Status Stm32CubeInitiator::Configure(const Config& config) {
  instance_data_->spi_handle.Init.DataSize = GetDataSize(config.bits_per_word);
  instance_data_->spi_handle.Init.FirstBit = GetBitOrder(config.bit_order);
  instance_data_->spi_handle.Init.CLKPhase = GetPhase(config.phase);
  instance_data_->spi_handle.Init.CLKPolarity = GetPolarity(config.polarity);
  PW_TRY(LazyInit());

  return OkStatus();
}

Status Stm32CubeInitiator::WriteRead(ConstByteSpan write_buffer,
                                     ByteSpan read_buffer) {
  PW_TRY(LazyInit());

  HAL_StatusTypeDef status;

  if (!write_buffer.empty()) {
    if (!read_buffer.empty()) {
      // TODO(cmumford): Not yet conforming to the WriteRead contract.
      uint16_t size = std::min(write_buffer.size(), read_buffer.size());
      status = HAL_SPI_TransmitReceive(
          &instance_data_->spi_handle,
          reinterpret_cast<uint8_t*>(
              const_cast<std::byte*>(write_buffer.data())),
          reinterpret_cast<uint8_t*>(read_buffer.data()),
          size,
          kTimeout);
    } else {
      status =
          HAL_SPI_Transmit(&instance_data_->spi_handle,
                           reinterpret_cast<uint8_t*>(
                               const_cast<std::byte*>(write_buffer.data())),
                           write_buffer.size(),
                           kTimeout);
      if (status != HAL_OK) {
        PW_LOG_ERROR("Stm32CubeInitiator::WriteRead: write:%ld B, s:%s",
                     write_buffer.size(),
                     ConvertStatus(status).str());
      }
    }
  } else {
    status = HAL_SPI_Receive(&instance_data_->spi_handle,
                             reinterpret_cast<uint8_t*>(read_buffer.data()),
                             read_buffer.size(),
                             kTimeout);
  }

  return ConvertStatus(status);
}

}  // namespace pw::spi
