# Copyright 2023 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""This package provides tooling specific to the stm32f769i-disc0 target."""

from stm32f769i_disc0_utils.unit_test_runner import TestingFailure
from stm32f769i_disc0_utils.unit_test_runner import flash_device
from stm32f769i_disc0_utils.unit_test_runner import run_device_test
from stm32f769i_disc0_utils.stm32f769i_detector import detect_boards