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

#include "pw_boot/boot.h"

#include "board.h"
#include "clock_config.h"
#include "fsl_clock.h"
#include "pin_mux.h"
#include "pw_boot_cortex_m/boot.h"
#include "pw_preprocessor/compiler.h"
#include "pw_sys_io_mcuxpresso/init.h"

#if PW_MALLOC_ACTIVE
#include "pw_malloc/malloc.h"
#endif  // PW_MALLOC_ACTIVE

void pw_boot_PreStaticMemoryInit() {
  // Call CMSIS SystemInit code.
  SystemInit();
}

void pw_boot_PreStaticConstructorInit() {
#if PW_MALLOC_ACTIVE
  pw_MallocInit(&pw_boot_heap_low_addr, &pw_boot_heap_high_addr);
#endif  // PW_MALLOC_ACTIVE
}

void pw_boot_PreMainInit() {
  CLOCK_AttachClk(kLPOSC_to_UTICK_CLK);

  BOARD_InitBootPins();

  BOARD_InitBootClocks();

  pw_sys_io_mcuxpresso_Init();
}

PW_NO_RETURN void pw_boot_PostMain() {
  // In case main() returns, just sit here until the device is reset.
  while (true) {
  }
  PW_UNREACHABLE;
}