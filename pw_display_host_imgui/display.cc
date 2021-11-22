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

namespace {

constexpr int kDisplayWidth = 320;
constexpr int kDisplayHeight = 240;
constexpr int kDisplayDataSize = kDisplayWidth * kDisplayHeight;

uint16_t internal_framebuffer[kDisplayDataSize];

// OpenGL texture data.
GLuint lcd_pixel_data[kDisplayDataSize];

// imgui state
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
GLuint lcd_texture = 0;
GLFWwindow* window;
int lcd_texture_display_scale = 2;
int old_lcd_texture_display_scale = 0;
bool lcd_texture_display_mode_nearest = true;
bool old_lcd_texture_display_mode_nearest = true;

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
  ColorRGBA c(rgb565);
  _SetTexturePixel(x, y, c.r, c.g, c.b, 255);
}

void UpdateLcdTexture(FramebufferRgb565* frame_buffer) {
  // Copy frame_buffer into lcd_pixel_data
  for (GLuint x = 0; x < kDisplayWidth; x++) {
    for (GLuint y = 0; y < kDisplayHeight; y++) {
      color_rgb565_t c = frame_buffer->GetPixel(x, y);
      _SetTexturePixel(x, y, c);
    }
  }

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
  glBindTexture(GL_TEXTURE_2D, NULL);
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

  glBindTexture(GL_TEXTURE_2D, NULL);

  *out_texture = image_texture;
}

static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

}  // namespace

namespace pw::display {

uint16_t* GetInternalFramebuffer() { return internal_framebuffer; }

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
  window = glfwCreateWindow(1280, 720, "pw_display", NULL, NULL);
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

void Update(pw::framebuffer::FramebufferRgb565* frame_buffer) {
  if (old_lcd_texture_display_mode_nearest !=
      lcd_texture_display_mode_nearest) {
    old_lcd_texture_display_mode_nearest = lcd_texture_display_mode_nearest;
    SetupLcdTexture(&lcd_texture);
  }
  UpdateLcdTexture(frame_buffer);

  // Poll and handle events (inputs, window resize, etc.)
  glfwPollEvents();

  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // 1. Show the big demo window (Most of the sample code is in
  // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
  // ImGui!).
  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair
  // to created a named window.
  {
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!"
                                    // and append into it.

    ImGui::Text("This is some useful text.");  // Display some text (you can use
                                               // a format strings too)
    ImGui::Checkbox(
        "Demo Window",
        &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float",
                       &f,
                       0.0f,
                       1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3(
        "clear color",
        (float*)&clear_color);  // Edit 3 floats representing a color

    if (ImGui::Button("Button"))  // Buttons return true when clicked (most
                                  // widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Begin("Display");
    ImGui::Text("Pixel Size = %d x %d", kDisplayWidth, kDisplayHeight);
    ImGui::Checkbox("Nearest neighbor", &lcd_texture_display_mode_nearest);
    ImGui::SliderInt("Integer Scaling", &lcd_texture_display_scale, 1, 10);
    ImGui::Image((void*)(intptr_t)lcd_texture,
                 ImVec2(lcd_texture_display_scale * kDisplayWidth,
                        lcd_texture_display_scale * kDisplayHeight));
    ImGui::End();
  }

  // 3. Show another simple window.
  if (show_another_window) {
    ImGui::Begin(
        "Another Window",
        &show_another_window);  // Pass a pointer to our bool variable (the
                                // window will have a closing button that will
                                // clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }

  // Rendering
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);

  if (glfwWindowShouldClose(window)) {
    CleanupAndExit();
  }
}

bool TouchscreenAvailable() { return false; }

bool NewTouchEvent() { return false; }

pw::coordinates::Vec3Int GetTouchPoint() {
  pw::coordinates::Vec3Int point;
  point.x = 0;
  point.y = 0;
  point.z = 0;
  return point;
}

}  // namespace pw::display
