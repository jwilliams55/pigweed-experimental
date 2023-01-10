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

#include "pw_display/display.h"

#include <array>
#include <utility>

#include "gtest/gtest.h"

using pw::color::color_rgb565_t;
using pw::display_driver::DisplayDriver;
using pw::framebuffer::FramebufferRgb565;

namespace pw::display {

namespace {

constexpr size_t kMaxSavedParams = 10;

enum class CallFunc {
  Unset,
  Update,
  WriteRow,
};

struct CallParams {
  CallFunc call_func = CallFunc::Unset;
  struct {
    color_rgb565_t* fb_data = nullptr;
  } update;
  struct {
    size_t num_pixels = 0;
    int row_idx = 0;
    int col_idx = 0;
  } write_row;
};

class TestDisplayDriver : public DisplayDriver {
 public:
  TestDisplayDriver(int width, int height) : width_(width), height_(height) {}
  virtual ~TestDisplayDriver() = default;

  Status Init() override { return OkStatus(); }

  Status Update(const FramebufferRgb565& framebuffer) override {
    if (next_call_param_idx_ < kMaxSavedParams) {
      call_params_[next_call_param_idx_].call_func = CallFunc::Update;
      call_params_[next_call_param_idx_].update.fb_data =
          framebuffer.GetFramebufferData();
      next_call_param_idx_++;
    }
    return OkStatus();
  }

  Status WriteRow(span<uint16_t> pixel_data,
                  int row_idx,
                  int col_idx) override {
    if (next_call_param_idx_ < kMaxSavedParams) {
      call_params_[next_call_param_idx_].call_func = CallFunc::WriteRow;
      call_params_[next_call_param_idx_].write_row.num_pixels =
          pixel_data.size();
      call_params_[next_call_param_idx_].write_row.row_idx = row_idx;
      call_params_[next_call_param_idx_].write_row.col_idx = col_idx;
      next_call_param_idx_++;
    }
    return OkStatus();
  }

  int GetWidth() const override { return width_; }

  int GetHeight() const override { return height_; }

  int GetNumCalls() const {
    int count = 0;
    for (size_t i = 0;
         i < kMaxSavedParams && call_params_[i].call_func != CallFunc::Unset;
         i++) {
      count++;
    }
    return count;
  }

  const CallParams& GetCall(size_t call_idx) {
    PW_ASSERT(call_idx <= call_params_.size());

    return call_params_[call_idx];
  }

 private:
  size_t next_call_param_idx_ = 0;
  std::array<CallParams, kMaxSavedParams> call_params_;
  const int width_;
  const int height_;
};

TEST(Display, ReleaseNoResize) {
  constexpr int kFramebufferWidth = 2;
  constexpr int kFramebufferHeight = 1;
  constexpr int kNumPixels = kFramebufferWidth * kFramebufferHeight;
  constexpr int kFramebufferRowBytes =
      sizeof(color_rgb565_t) * kFramebufferWidth;
  color_rgb565_t pixel_data[kNumPixels];

  TestDisplayDriver test_driver(kFramebufferWidth, kFramebufferHeight);
  Display display(FramebufferRgb565(pixel_data,
                                    kFramebufferWidth,
                                    kFramebufferHeight,
                                    kFramebufferRowBytes),
                  test_driver);
  FramebufferRgb565 fb = display.GetFramebuffer();
  EXPECT_TRUE(fb.IsValid());
  EXPECT_EQ(kFramebufferWidth, fb.GetWidth());
  EXPECT_EQ(kFramebufferHeight, fb.GetHeight());
  EXPECT_EQ(0, test_driver.GetNumCalls());

  display.ReleaseFramebuffer(std::move(fb));
  ASSERT_EQ(1, test_driver.GetNumCalls());
  auto call = test_driver.GetCall(0);
  EXPECT_EQ(CallFunc::Update, call.call_func);
  EXPECT_EQ(pixel_data, call.update.fb_data);
}

TEST(Display, ReleaseSmallResize) {
  constexpr int kDisplayWidth = 8;
  constexpr int kDisplayHeight = 4;
  constexpr int kFramebufferWidth = 2;
  constexpr int kFramebufferHeight = 1;
  constexpr int kNumPixels = kFramebufferWidth * kFramebufferHeight;
  constexpr int kFramebufferRowBytes =
      sizeof(color_rgb565_t) * kFramebufferWidth;
  color_rgb565_t pixel_data[kNumPixels];

  TestDisplayDriver test_driver(kDisplayWidth, kDisplayHeight);
  Display display(FramebufferRgb565(pixel_data,
                                    kFramebufferWidth,
                                    kFramebufferHeight,
                                    kFramebufferRowBytes),
                  test_driver);
  FramebufferRgb565 fb = display.GetFramebuffer();
  EXPECT_TRUE(fb.IsValid());
  EXPECT_EQ(kFramebufferWidth, fb.GetWidth());
  EXPECT_EQ(kFramebufferHeight, fb.GetHeight());
  EXPECT_EQ(0, test_driver.GetNumCalls());

  display.ReleaseFramebuffer(std::move(fb));
  ASSERT_EQ(4, test_driver.GetNumCalls());
  auto call = test_driver.GetCall(0);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(8U, call.write_row.num_pixels);
  EXPECT_EQ(0, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(1);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(8U, call.write_row.num_pixels);
  EXPECT_EQ(1, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(2);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(8U, call.write_row.num_pixels);
  EXPECT_EQ(2, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(3);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(8U, call.write_row.num_pixels);
  EXPECT_EQ(3, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);
}

TEST(Display, ReleaseWideResize) {
  // Display width > resize buffer (80 px.) will cause two writes per row.
  constexpr int kDisplayWidth = 90;
  constexpr int kDisplayHeight = 4;
  constexpr int kFramebufferWidth = 2;
  constexpr int kFramebufferHeight = 1;
  constexpr int kNumPixels = kFramebufferWidth * kFramebufferHeight;
  constexpr int kFramebufferRowBytes =
      sizeof(color_rgb565_t) * kFramebufferWidth;
  color_rgb565_t pixel_data[kNumPixels];

  TestDisplayDriver test_driver(kDisplayWidth, kDisplayHeight);
  Display display(FramebufferRgb565(pixel_data,
                                    kFramebufferWidth,
                                    kFramebufferHeight,
                                    kFramebufferRowBytes),
                  test_driver);
  FramebufferRgb565 fb = display.GetFramebuffer();
  EXPECT_TRUE(fb.IsValid());
  EXPECT_EQ(kFramebufferWidth, fb.GetWidth());
  EXPECT_EQ(kFramebufferHeight, fb.GetHeight());
  EXPECT_EQ(0, test_driver.GetNumCalls());

  display.ReleaseFramebuffer(std::move(fb));
  ASSERT_EQ(8, test_driver.GetNumCalls());
  auto call = test_driver.GetCall(0);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(80U, call.write_row.num_pixels);
  EXPECT_EQ(0, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(1);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(10U, call.write_row.num_pixels);
  EXPECT_EQ(0, call.write_row.row_idx);
  EXPECT_EQ(80, call.write_row.col_idx);

  call = test_driver.GetCall(2);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(80U, call.write_row.num_pixels);
  EXPECT_EQ(1, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(3);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(10U, call.write_row.num_pixels);
  EXPECT_EQ(1, call.write_row.row_idx);
  EXPECT_EQ(80, call.write_row.col_idx);

  call = test_driver.GetCall(4);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(80U, call.write_row.num_pixels);
  EXPECT_EQ(2, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(5);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(10U, call.write_row.num_pixels);
  EXPECT_EQ(2, call.write_row.row_idx);
  EXPECT_EQ(80, call.write_row.col_idx);

  call = test_driver.GetCall(6);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(80U, call.write_row.num_pixels);
  EXPECT_EQ(3, call.write_row.row_idx);
  EXPECT_EQ(0, call.write_row.col_idx);

  call = test_driver.GetCall(7);
  EXPECT_EQ(CallFunc::WriteRow, call.call_func);
  EXPECT_EQ(10U, call.write_row.num_pixels);
  EXPECT_EQ(3, call.write_row.row_idx);
  EXPECT_EQ(80, call.write_row.col_idx);
}

}  // namespace

}  // namespace pw::display
