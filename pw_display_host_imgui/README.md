# pw_display_host_imgui

## Setup Instructions

1. Install [ImGui](https://github.com/ocornut/imgui) and [glfw](https://www.glfw.org/).

   ```
   pw package install imgui
   pw package install glfw
   ```

2. Install host OS requiremets (only required for Linux) shown below.

## ImGui Requirements

### Linux

- Debian / Ubuntu

  ```sh
  sudo apt install libglfw3-dev libglfw3
  ```

- Arch Linux

  ```sh
  sudo pacman -S glfw-x11
  ```

3. Compile with:

```
gn gen out --args="
dir_pw_third_party_imgui=\"$PWD/.environment/packages/imgui\"
"
ninja -C out host
./out/host_debug/obj/applications/terminal_display/bin/terminal_demo
```
