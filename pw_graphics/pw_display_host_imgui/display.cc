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

// LCD Facade using imgui running on a host machine.
// Much of this code is from the imgui example:
// https://github.com/ocornut/imgui/tree/master/examples/example_glfw_opengl3
// As well as the wiki page:
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
#include "pw_display/display.h"

// To silence large number of warnings (at least on macOS).
#define GL_SILENCE_DEPRECATION

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>  // Will drag system OpenGL headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cinttypes>
#include <cstdint>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "pw_color/color.h"
#include "pw_color/colors_pico8.h"
#include "pw_coordinates/vec_int.h"
#include "pw_display/display.h"
#include "pw_framebuffer/rgb565.h"

using pw::color::color_rgb565_t;

namespace {

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

// OpenGL texture data.
GLuint lcd_pixel_data[kDisplayDataSize];

// imgui state
bool show_imgui_demo_window = false;
ImVec4 clear_color = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
GLuint lcd_texture = 0;
GLFWwindow* window;
int lcd_texture_display_scale = 3;
int old_lcd_texture_display_scale = 0;
bool lcd_texture_display_mode_nearest = true;
bool old_lcd_texture_display_mode_nearest = true;

bool left_mouse_pressed = false;
int texture_mouse_x = 0;
int texture_mouse_y = 0;
uint16_t framebuffer_data[kDisplayDataSize];

void CleanupAndExit() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(0);
}

void _SetTexturePixel(
    GLuint x, GLuint y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  // Calculate target color
  GLuint target_color;
  GLubyte* colors = (GLubyte*)&target_color;
  colors[0] = r;
  colors[1] = g;
  colors[2] = b;
  colors[3] = a;
  lcd_pixel_data[y * kDisplayWidth + x] = target_color;
}

void _SetTexturePixel(GLuint x, GLuint y, uint8_t r, uint8_t g, uint8_t b) {
  _SetTexturePixel(x, y, r, g, b, 255);
}

void _SetTexturePixel(GLuint x, GLuint y, color_rgb565_t rgb565) {
  pw::color::ColorRGBA c(rgb565);
  _SetTexturePixel(x, y, c.r, c.g, c.b, 255);
}

void UpdateLcdTexture() {
  // Set current texture
  glBindTexture(GL_TEXTURE_2D, lcd_texture);
  // Update texture
  glTexSubImage2D(GL_TEXTURE_2D,
                  0,
                  0,
                  0,
                  kDisplayWidth,
                  kDisplayHeight,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  lcd_pixel_data);
  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);
}

void SetupLcdTexture(GLuint* out_texture) {
  // Create a OpenGL texture identifier
  GLuint image_texture;
  glGenTextures(1, &image_texture);
  glBindTexture(GL_TEXTURE_2D, image_texture);

  // Setup filtering parameters for display
  GLuint display_mode =
      lcd_texture_display_mode_nearest ? GL_NEAREST : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, display_mode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, display_mode);
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_WRAP_S,
                  GL_CLAMP_TO_EDGE);  // This is required on WebGL for non
                                      // power-of-two textures
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // Same

  // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               kDisplayWidth,
               kDisplayHeight,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               lcd_pixel_data);

  glBindTexture(GL_TEXTURE_2D, 0);

  *out_texture = image_texture;
}

static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

}  // namespace

namespace pw::display {

int GetWidth() { return kDisplayWidth; }
int GetHeight() { return kDisplayHeight; }

void Init() {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Create window with graphics context
  window = glfwCreateWindow(1280, 800, "pw_display", NULL, NULL);
  if (window == NULL)
    return;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  // io.Fonts->AddFontFromFileTTF("NotoSans-Regular.ttf", 32.0);

  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // Enable Gamepad Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  SetupLcdTexture(&lcd_texture);
}

void RecreateLcdTexture() {
  if (old_lcd_texture_display_mode_nearest !=
      lcd_texture_display_mode_nearest) {
    old_lcd_texture_display_mode_nearest = lcd_texture_display_mode_nearest;
    SetupLcdTexture(&lcd_texture);
  }
}

void Render();

void UpdatePixelDouble(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  RecreateLcdTexture();

  // Copy frame_buffer into lcd_pixel_data
  int basex, basey;
  for (GLuint y = 0; y < frame_buffer->GetHeight(); y++) {
    for (GLuint x = 0; x < frame_buffer->GetWidth(); x++) {
      if (auto c = frame_buffer->GetPixel(x, y); c.ok()) {
        basex = x * 2;
        basey = y * 2;
        _SetTexturePixel(basex, basey, c.value());
        _SetTexturePixel(basex + 1, basey, c.value());
        _SetTexturePixel(basex, basey + 1, c.value());
        _SetTexturePixel(basex + 1, basey + 1, c.value());
      }
    }
  }

  Render();
}

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  RecreateLcdTexture();

  // Copy frame_buffer into lcd_pixel_data
  for (GLuint x = 0; x < kDisplayWidth; x++) {
    for (GLuint y = 0; y < kDisplayHeight; y++) {
      if (auto c = frame_buffer->GetPixel(x, y); c.ok()) {
        _SetTexturePixel(x, y, c.value());
      }
    }
  }

  Render();
}

void Render() {
  UpdateLcdTexture();

  // Poll and handle events (inputs, window resize, etc.)
  glfwPollEvents();

  left_mouse_pressed = false;
  double mouse_xpos = 0, mouse_ypos = 0;
  glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    left_mouse_pressed = true;
  }

  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);

  // 1. Show the big demo window (Most of the sample code is in
  // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
  // ImGui!).
  if (show_imgui_demo_window) {
    ImGui::ShowDemoWindow(&show_imgui_demo_window);
  }

  // Calculate the display texture draw coordinates
  int scaled_width = kDisplayWidth * lcd_texture_display_scale;
  int scaled_height = kDisplayHeight * lcd_texture_display_scale;

  float offset_x = round((display_w - scaled_width) / 2.0);
  float offset_y = round((display_h - scaled_height) / 2.0);

  ImGui::Begin("Pigweed Display");
  ImGui::Text("Pixel Size = %d x %d", kDisplayWidth, kDisplayHeight);
  ImGui::SliderInt("Integer Scaling", &lcd_texture_display_scale, 1, 10);
  ImGui::Checkbox("Nearest neighbor", &lcd_texture_display_mode_nearest);
  // Show the display in the Imgui window itself:
  // ImGui::Image((void*)(intptr_t)lcd_texture,
  //              ImVec2(lcd_texture_display_scale * kDisplayWidth,
  //                     lcd_texture_display_scale * kDisplayHeight));

  texture_mouse_x = round((mouse_xpos - offset_x) / lcd_texture_display_scale);
  texture_mouse_y = round((mouse_ypos - offset_y) / lcd_texture_display_scale);
  ImGui::Separator();
  ImGui::Text("Mouse position = %d, %d", texture_mouse_x, texture_mouse_y);
  ImGui::Text("Mouse Left button pressed: %d", left_mouse_pressed);

  ImGui::Separator();
  ImGui::Checkbox("Show Imgui Demo Window", &show_imgui_demo_window);
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);

  ImGui::End();

  // Rendering
  ImGui::Render();

  glViewport(0, 0, display_w, display_h);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, display_w, display_h, 0.0f, -1.0f, 1.0f);

  // Clear the screen
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw the display texture
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glPushMatrix();

  glBindTexture(GL_TEXTURE_2D, lcd_texture);
  glEnable(GL_TEXTURE_2D);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex2f(offset_x, offset_y);
  glTexCoord2f(0.0, 1.0);
  glVertex2f(offset_x, offset_y + scaled_height);
  glTexCoord2f(1.0, 1.0);
  glVertex2f(offset_x + scaled_width, offset_y + scaled_height);
  glTexCoord2f(1.0, 0.0);
  glVertex2f(offset_x + scaled_width, offset_y);
  glEnd();

  glPopMatrix();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);

  if (glfwWindowShouldClose(window)) {
    CleanupAndExit();
  }
}

bool TouchscreenAvailable() { return true; }

bool NewTouchEvent() { return left_mouse_pressed; }

pw::coordinates::Vec3Int GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  if (left_mouse_pressed && texture_mouse_x >= 0 &&
      texture_mouse_x < kDisplayWidth && texture_mouse_y >= 0 &&
      texture_mouse_y < kDisplayHeight) {
    point.x = texture_mouse_x;
    point.y = texture_mouse_y;
    point.z = 1;
  }
  return point;
}

Status InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(
      framebuffer_data, kDisplayWidth, kDisplayHeight);
  return OkStatus();
}

}  // namespace pw::display
