# GLFW Library

This folder holds the GN build file for the [GLFW](https://www.glfw.org/)
library.


## Setup Instructions

1. Install [glfw](https://www.glfw.org/).

   ```
   pw package install glfw
   ```

2. Install host OS requirements (only required for Linux) shown below.

### Linux

- Debian / Ubuntu

  ```sh
  sudo apt install libglfw3-dev libglfw3
  ```

- Arch Linux

  ```sh
  sudo pacman -S glfw-x11
  ```

# Building

The build file requires the `dir_pw_third_party_glfw` be set.
This can be done as so:

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_glfw=\"//environment/packages/glfw\"
"
ninja -C out
```
