# Pigweed Experimental

[TOC]

This repository contains experimental pigweed modules.

## Repository setup

Clone this repo with `--recursive` to get all required submodules.

```sh
git clone --recursive https://pigweed.googlesource.com/pigweed/experimental
```

This will pull the [Pigweed source
repository](https://pigweed.googlesource.com/pigweed/pigweed) into
`third_party/pigweed`. If you already cloned but forgot to `--recursive` run
`git submodule update --init` to pull all submodules.


## pw_graphics

The [//pw_graphics](/pw_graphics) folder contains some libraries for drawing to
an RGB565 framebuffer and displaying it on various platforms.

The demo applications that make use of these libraries are:
- [//applications/terminal_display](/applications/terminal_display)

### Build instructions

First time setup:

```
git clone --recursive https://pigweed.googlesource.com/pigweed/experimental
cd experimental
. ./bootstrap.sh
pw package install imgui
pw package install glfw
pw package install stm32cube_f4
```

#### **[STM32F429-DISC1](https://www.st.com/en/evaluation-tools/32f429idiscovery.html)**

Compile:

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_stm32cube_f4=\"$PROJECT_DIR/.environment/packages/stm32cube_f4\"
"
ninja -C out
```

Flash:

```
openocd -f third_party/pigweed/targets/stm32f429i_disc1/py/stm32f429i_disc1_utils/openocd_stm32f4xx.cfg -c "program out/stm32f429i_disc1_stm32cube_debug/obj/applications/terminal_display/bin/terminal_demo.elf verify reset exit"
```

#### **Linux, Windows or Mac**

Compile:

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_imgui=\"$PROJECT_DIR/.environment/packages/imgui\"
"
ninja -C out
```

Run:

```
out/host_debug/obj/applications/terminal_display/bin/terminal_demo
```

#### **[Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) Connected to an [ILI9341](https://www.adafruit.com/?q=ili9341&sort=BestMatch)**

Clone the pico-sdk repo:
```
cd $HOME
git clone https://github.com/raspberrypi/pico-sdk
```

Compile:

```sh
gn gen out --export-compile-commands --args="
  PICO_SRC_DIR=\"$HOME/pico-sdk\"
"
ninja -C out
```

Create a uf2 file for flashing the Pico with:

```sh
./out/host_debug/obj/targets/rp2040/bin/elf2uf2 ./out/rp2040/obj/applications/terminal_display/bin/terminal_demo.elf ./out/rp2040/obj/applications/terminal_display/bin/terminal_demo.uf2
```

Copy `./out/rp2040/obj/applications/terminal_display/bin/terminal_demo.uf2` to your Pi Pico.
