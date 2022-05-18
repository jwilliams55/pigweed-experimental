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

#include <cinttypes>

#include "pw_color/color.h"

namespace pw::color {

const color_rgb565_t colors_pico8_rgb565[] = {
    ColorRGBA(0x00, 0x00, 0x00).ToRgb565(),  // 0x0000, // #000000 0 BLACK
    ColorRGBA(0x1d, 0x2b, 0x53).ToRgb565(),  // 0x194a, // #1d2b53 1 DARK_BLUE
    ColorRGBA(0x7e, 0x25, 0x53).ToRgb565(),  // 0x792a, // #7e2553 2 DARK_PURPLE
    ColorRGBA(0x00, 0x87, 0x51).ToRgb565(),  // 0x042a, // #008751 3 DARK_GREEN
    ColorRGBA(0xab, 0x52, 0x36).ToRgb565(),  // 0xaa86, // #ab5236 4 BROWN
    ColorRGBA(0x5f, 0x57, 0x4f).ToRgb565(),  // 0x5aa9, // #5f574f 5 DARK_GRAY
    ColorRGBA(0xc2, 0xc3, 0xc7).ToRgb565(),  // 0xc618, // #c2c3c7 6 LIGHT_GRAY
    ColorRGBA(0xff, 0xf1, 0xe8).ToRgb565(),  // 0xff9d, // #fff1e8 7 WHITE
    ColorRGBA(0xff, 0x00, 0x4d).ToRgb565(),  // 0xf809, // #ff004d 8 RED
    ColorRGBA(0xff, 0xa3, 0x00).ToRgb565(),  // 0xfd00, // #ffa300 9 ORANGE
    ColorRGBA(0xff, 0xec, 0x27).ToRgb565(),  // 0xff64, // #ffec27 10 YELLOW
    ColorRGBA(0x00, 0xe4, 0x36).ToRgb565(),  // 0x0726, // #00e436 11 GREEN
    ColorRGBA(0x29, 0xad, 0xff).ToRgb565(),  // 0x2d7f, // #29adff 12 BLUE
    ColorRGBA(0x83, 0x76, 0x9c).ToRgb565(),  // 0x83b3, // #83769c 13 INDIGO
    ColorRGBA(0xff, 0x77, 0xa8).ToRgb565(),  // 0xfbb5, // #ff77a8 14 PINK
    ColorRGBA(0xff, 0xcc, 0xaa).ToRgb565(),  // 0xfe75, // #ffccaa 15 PEACH
};

const color_rgba8888_t colors_pico8_rgba8888[] = {
    0xff000000,  // #000000 0  BLACK
    0xff532b1d,  // #1d2b53 1  DARK_BLUE
    0xff53257e,  // #7e2553 2  DARK_PURPLE
    0xff518700,  // #008751 3  DARK_GREEN
    0xff3652ab,  // #ab5236 4  BROWN
    0xff4f575f,  // #5f574f 5  DARK_GRAY
    0xffc7c3c2,  // #c2c3c7 6  LIGHT_GRAY
    0xffe8f1ff,  // #fff1e8 7  WHITE
    0xff4d00ff,  // #ff004d 8  RED
    0xff00a3ff,  // #ffa300 9  ORANGE
    0xff27ecff,  // #ffec27 10 YELLOW
    0xff36e400,  // #00e436 11 GREEN
    0xffffad29,  // #29adff 12 BLUE
    0xff9c7683,  // #83769c 13 INDIGO
    0xffa877ff,  // #ff77a8 14 PINK
    0xffaaccff,  // #ffccaa 15 PEACH
};

#define COLOR_BLACK 0
#define COLOR_DARK_BLUE 1
#define COLOR_DARK_PURPLE 2
#define COLOR_DARK_GREEN 3
#define COLOR_BROWN 4
#define COLOR_DARK_GRAY 5
#define COLOR_LIGHT_GRAY 6
#define COLOR_WHITE 7
#define COLOR_RED 8
#define COLOR_ORANGE 9
#define COLOR_YELLOW 10
#define COLOR_GREEN 11
#define COLOR_BLUE 12
#define COLOR_INDIGO 13
#define COLOR_PINK 14
#define COLOR_PEACH 16

}  // namespace pw::color
