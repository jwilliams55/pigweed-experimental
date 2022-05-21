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

#include <math.h>

constexpr float kPi = 3.1415926535897932384626433832795;
constexpr float kHalfPi = 1.5707963267948966192313216916398;
constexpr float kTwoPi = 6.283185307179586476925286766559;
constexpr float DegreesToRadians = 0.017453292519943295769236907684886;
constexpr float RadiansToDegrees = 57.295779513082320876798154814105;

#define radians(deg) ((deg)*DegreesToRadians)
#define degrees(rad) ((rad)*RadiansToDegrees)

class Vec2 {
 public:
  float x, y;

  Vec2() : x(0), y(0) {}
  Vec2(float newx, float newy) : x(newx), y(newy) {}
  Vec2(const Vec2& v) : x(v.x), y(v.y) {}

  Vec2& operator=(const Vec2& v) {
    x = v.x;
    y = v.y;
    return *this;
  }

  Vec2 operator+(Vec2& v) { return Vec2(x + v.x, y + v.y); }
  Vec2 operator-(Vec2& v) { return Vec2(x - v.x, y - v.y); }

  Vec2& operator+=(Vec2& v) {
    x += v.x;
    y += v.y;
    return *this;
  }
  Vec2& operator-=(Vec2& v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }

  Vec2 operator+(float s) { return Vec2(x + s, y + s); }
  Vec2 operator-(float s) { return Vec2(x - s, y - s); }
  Vec2 operator*(float s) { return Vec2(x * s, y * s); }
  Vec2 operator/(float s) { return Vec2(x / s, y / s); }

  Vec2& operator+=(float s) {
    x += s;
    y += s;
    return *this;
  }
  Vec2& operator-=(float s) {
    x -= s;
    y -= s;
    return *this;
  }
  Vec2& operator*=(float s) {
    x *= s;
    y *= s;
    return *this;
  }
  Vec2& operator/=(float s) {
    x /= s;
    y /= s;
    return *this;
  }

  void set(float newx, float newy) {
    this->x = newx;
    this->y = newy;
  }

  // Rotate the vector by theta radians.
  void rotate(float theta) {
    float c = cosf(theta);
    float s = sinf(theta);
    float tx = x * c - y * s;
    float ty = x * s + y * c;
    x = tx;
    y = ty;
  }

  Vec2& normalize() {
    float len = length();
    if (len == 0)
      return *this;
    *this *= (1.0 / len);
    return *this;
  }

  float dist(Vec2 v) const {
    Vec2 d(v.x - x, v.y - y);
    return d.length();
  }

  float length() const { return sqrtf(x * x + y * y); }

  // Return the inverse tangent of y/x in radians.
  float angle() const { return atan2f(y, x); }

  Vec2 perpendicular() const { return Vec2(-y, x); }

  static float dot(Vec2 v1, Vec2 v2) { return v1.x * v2.x + v1.y * v2.y; }
  static float cross(Vec2 v1, Vec2 v2) { return (v1.x * v2.y) - (v1.y * v2.x); }
};
