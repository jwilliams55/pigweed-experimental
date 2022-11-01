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
#include "pw_display/display_backend.h"

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
using pw::framebuffer::FramebufferRgb565;

namespace pw::display::backend {

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

static void HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
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

int Display::GetWidth() const { return kDisplayWidth; }

int Display::GetHeight() const { return kDisplayHeight; }

Status Display::Init() {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return Status::Internal();

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
    return Status::Internal();
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
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
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
  return OkStatus();
}

void RecreateLcdTexture() {
  if (old_lcd_texture_display_mode_nearest !=
      lcd_texture_display_mode_nearest) {
    old_lcd_texture_display_mode_nearest = lcd_texture_display_mode_nearest;
    SetupLcdTexture(&lcd_texture);
  }
}

void Render();

Display::Display() = default;

Display::~Display() = default;

void UpdatePixelDouble(FramebufferRgb565* frame_buffer) {
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

void Display::Update(FramebufferRgb565& frame_buffer) {
  RecreateLcdTexture();

  // Copy frame_buffer into lcd_pixel_data
  for (GLuint x = 0; x < kDisplayWidth; x++) {
    for (GLuint y = 0; y < kDisplayHeight; y++) {
      if (auto c = frame_buffer.GetPixel(x, y); c.ok()) {
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
  ImGuiIO& io = ImGui::GetIO();

  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);

  // Build the empty dockspace background

  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  // 0px padding around the dockspace
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  // Start docspace window
  ImGui::Begin("DockSpace", NULL, window_flags);

  // Create empty dockspace container
  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

  ImGui::PopStyleVar();   // Padding around docspace window
  ImGui::PopStyleVar(2);  // WindowRounding and WindowBorderSize

  // Dockspace menu
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("Options")) {
      ImGui::MenuItem("Show Imgui Demo Window", "", &show_imgui_demo_window);
      ImGui::Separator();
      if (ImGui::MenuItem("Quit")) {
        CleanupAndExit();
      }
      ImGui::EndMenu();
    }
    HelpMarker(
        "Window Docking: "
        "\n"
        "- Drag from window title bar or their tab to dock/undock."
        "\n"
        "- Drag from window menu button (upper-left button) to undock an "
        "entire node (all windows)."
        "\n"
        "- Hold SHIFT to disable docking");

    ImGui::EndMenuBar();
  }

  ImGui::End();  // end dockspace

  // 1. Show the big demo window (Most of the sample code is in
  // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
  // ImGui!).
  if (show_imgui_demo_window) {
    ImGui::ShowDemoWindow(&show_imgui_demo_window);
  }

  // Draw Screen(s)

  // Screen 1 Window
  ImGui::Begin("Screen 1");
  // Calculate the display texture draw coordinates
  int scaled_width = kDisplayWidth * lcd_texture_display_scale;
  int scaled_height = kDisplayHeight * lcd_texture_display_scale;

  ImVec2 mouse_pos = ImGui::GetCursorScreenPos();
  ImVec2 mouse_coordinates_of_base_image;
  ImGui::Image((void*)(intptr_t)lcd_texture,
               ImVec2(scaled_width, scaled_height),
               // Top left texure coord
               ImVec2(0.0f, 0.0f),
               // Lower right texture coord
               ImVec2(1.0f, 1.0f),
               // Tint (none applied)
               ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
               // Border color (50% white)
               ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
  mouse_coordinates_of_base_image.x =
      floor((io.MousePos.x - mouse_pos.x) / lcd_texture_display_scale);
  mouse_coordinates_of_base_image.y =
      floor((io.MousePos.y - mouse_pos.y) / lcd_texture_display_scale);
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text("mouse coords = %.1f, %.1f",
                mouse_coordinates_of_base_image.x,
                mouse_coordinates_of_base_image.y);
    ImGui::EndTooltip();
  }
  ImGui::End();  // Screen 1 Window

  // For reference this forces the "Screen 1" window to be docked to the main
  // dockspace:
  //   ImGui::DockBuilderDockWindow("Screen 1", dockspace_id);

  ImGui::Begin("Screen 1 Settings");
  ImGui::Text("Pixel Size = %d x %d", kDisplayWidth, kDisplayHeight);
  ImGui::SliderInt("Integer Scaling", &lcd_texture_display_scale, 1, 10);
  ImGui::Checkbox("Nearest neighbor", &lcd_texture_display_mode_nearest);

  ImGui::Separator();
  texture_mouse_x = mouse_coordinates_of_base_image.x;
  texture_mouse_y = mouse_coordinates_of_base_image.y;
  ImGui::Text("Mouse position = %d, %d", texture_mouse_x, texture_mouse_y);
  ImGui::Text("Mouse Left button pressed: %d", left_mouse_pressed);

  // Demo Window toggle
  ImGui::Separator();

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);

  ImGui::End();

  // Done building the UI
  ImGui::Render();

  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  // Render ImGui
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);

  if (glfwWindowShouldClose(window)) {
    CleanupAndExit();
  }
}

bool Display::TouchscreenAvailable() const { return true; }

bool Display::NewTouchEvent() { return left_mouse_pressed; }

pw::coordinates::Vec3Int Display::GetTouchPoint() {
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

Status Display::InitFramebuffer(FramebufferRgb565* framebuffer) {
  framebuffer->SetFramebufferData(framebuffer_data,
                                  kDisplayWidth,
                                  kDisplayHeight,
                                  kDisplayWidth * sizeof(uint16_t));
  return OkStatus();
}

}  // namespace pw::display::backend
