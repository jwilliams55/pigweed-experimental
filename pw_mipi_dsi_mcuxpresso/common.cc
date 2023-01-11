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

#include "common.h"

namespace pw::mipi::dsi {

Status MCUXpressoToPigweedStatus(status_t mcux_status) {
  switch (mcux_status) {
    case kStatus_Success:
      return OkStatus();
    case kStatus_Fail:
      return Status::Internal();
    case kStatus_ReadOnly:
      return Status::PermissionDenied();
    case kStatus_OutOfRange:
      return Status::OutOfRange();
    case kStatus_InvalidArgument:
      return Status::InvalidArgument();
    case kStatus_Timeout:
      return Status::DeadlineExceeded();
    case kStatus_NoTransferInProgress:
      return Status::FailedPrecondition();
    case kStatus_Busy:
      return Status::FailedPrecondition();
    case kStatus_NoData:
      return Status::Unavailable();
  }
  return Status::Internal();
}

}  // namespace pw::mipi::dsi
