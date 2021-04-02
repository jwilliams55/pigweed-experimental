// Copyright 2021 The Pigweed Authors
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

// Use Default config template, which includes the entire HAL
//  + some defaults (i.e. Ethernet MAC Address, oscillator config)
// Some configs, i.e. HSE_VALUE are set in the BUILD.gn through cflags.
#include "stm32f4xx_hal_conf_template.h"
