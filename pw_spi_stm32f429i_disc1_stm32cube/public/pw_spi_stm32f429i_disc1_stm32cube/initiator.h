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

#include "pw_spi/initiator.h"
#include "pw_status/status.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

namespace pw::spi {

// STM32 userspace implementation of the SPI Initiator
class Stm32CubeInitiator : public Initiator {
 public:
  Stm32CubeInitiator();
  ~Stm32CubeInitiator();

  // Implements pw::spi::Initiator
  Status Configure(const Config& config) override;
  Status WriteRead(ConstByteSpan write_buffer, ByteSpan read_buffer) override;

 private:
  Status LazyInit();
  Status InitSPI();

  bool initialized = false;
  pw::Status init_status;  // The saved LazyInit() status.
  SPI_HandleTypeDef spi_handle;
};

}  // namespace pw::spi
