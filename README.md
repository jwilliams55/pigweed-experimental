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

**First time setup:**

```
git clone --recursive https://pigweed.googlesource.com/pigweed/experimental
cd experimental
. ./bootstrap.sh
pw package install imgui
pw package install glfw
pw package install stm32cube_f4
pw package install pico_sdk
```

#### **[STM32F429-DISC1](https://www.st.com/en/evaluation-tools/32f429idiscovery.html)**

**Compile:**

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_stm32cube_f4=\"$PW_PROJECT_ROOT/environment/packages/stm32cube_f4\"
"
ninja -C out
```

**Flash:**

```
openocd -f third_party/pigweed/targets/stm32f429i_disc1/py/stm32f429i_disc1_utils/openocd_stm32f4xx.cfg -c "program out/stm32f429i_disc1_stm32cube_debug/obj/applications/terminal_display/bin/terminal_demo.elf verify reset exit"
```

#### **[STM32F769-DISC0](https://www.st.com/en/evaluation-tools/32f769idiscovery.html)**

**First time setup:**
```
pw package install stm32cube_f7
```

**Compile:**

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_stm32cube_f7=\"//environment/packages/stm32cube_f7\"
"
ninja -C out
```

**Flash:**

```
openocd -f targets/stm32f769i_disc0/py/stm32f769i_disc0_utils/openocd_stm32f7xx.cfg \
  -c "program out/stm32f769i_disc0_debug/obj/applications/blinky/bin/blinky.elf verify reset exit"
```

#### **Linux, Windows or Mac**

**Compile:**

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_imgui=\"$PW_PROJECT_ROOT/environment/packages/imgui\"
  dir_pw_third_party_glfw=\"$PW_PROJECT_ROOT/environment/packages/glfw\"
"
ninja -C out
```

**Run:**

```
out/host_debug/obj/applications/terminal_display/bin/terminal_demo
```

#### **[Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) Connected to an external SPI display**

Working displays:

- [ILI9341](https://www.adafruit.com/?q=ili9341&sort=BestMatch)
- [Pico Display Pack 2.0 (ST7789)](https://shop.pimoroni.com/products/pico-display-pack-2-0)

**First time setup:**
```
pw package install pico_sdk
```

**Compile:**

```sh
gn gen out --export-compile-commands --args='
  PICO_SRC_DIR="//environment/packages/pico_sdk"
'
ninja -C out
```

**Flash:**

- Using a uf2 file:

  1. Reboot pico into BOOTSEL mode by holding the bootsel button on startup.
  2. Copy `./out/rp2040/obj/applications/terminal_display/terminal_demo.uf2` to your Pi Pico.

- Using `picotool`:

  1. Reboot pico into BOOTSEL mode by holding the bootsel button on startup. Or try forcing a reboot with `picotool`:

  ```sh
  picotool reboot -f -u
  ```

  2. Flash the elf or uf2 file and reboot.

  ```sh
  picotool load ./out/rp2040/obj/applications/terminal_display/bin/terminal_demo.elf
  picotool reboot
  ```

- Using a Pico Probe and openocd:

  This requires installing the Raspberry Pi foundation's OpenOCD fork for the
  Pico probe. More details including how to connect the two Pico boards is available at [Raspberry Pi Pico and RP2040 - C/C++ Part 2: Debugging with VS Code](https://www.digikey.com/en/maker/projects/raspberry-pi-pico-and-rp2040-cc-part-2-debugging-with-vs-code/470abc7efb07432b82c95f6f67f184c0)

  **Install RaspberryPi's OpenOCD Fork:**

  ```sh
  git clone https://github.com/raspberrypi/openocd.git \
    --branch picoprobe \
    --depth=1 \
    --no-single-branch \
    openocd-picoprobe

  cd openocd-picoprobe

  ./bootstrap
  ./configure --enable-picoprobe --prefix=$HOME/apps/openocd --disable-werror
  make -j2
  make install
  ```

  **Setup udev rules (Linux only):**

  ```sh
  cat <<EOF > 49-picoprobe.rules
  SUBSYSTEMS=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0004", MODE:="0666"
  KERNEL=="ttyACM*", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0004", MODE:="0666"
  EOF
  sudo cp 49-picoprobe.rules /usr/lib/udev/rules.d/49-picoprobe.rules
  sudo udevadm control --reload-rules
   ```

  **Flash the Pico:**

  ```sh
  ~/apps/openocd/bin/openocd -f ~/apps/openocd/share/openocd/scripts/interface/picoprobe.cfg -f ~/apps/openocd/share/openocd/scripts/target/rp2040.cfg -c 'program out/rp2040/obj/applications/terminal_display/bin/terminal_demo.elf verify reset exit'
  ```

  **Launching gdb***

  ```sh
  ~/apps/openocd/bin/openocd -f ~/apps/openocd/share/openocd/scripts/interface/picoprobe.cfg -f ~/apps/openocd/share/openocd/scripts/target/rp2040.cfg
  ```

  ```sh
  gdb-multiarch -ex "target remote :3333" -ex "set print pretty on" out/rp2040/obj/applications/terminal_display/bin/terminal_demo.elf
  ```

  `arm-none-eabi-gdb` can be used in place of `gdb-multiarch` above.

#### **[MIMXRT595-EVK](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/i-mx-rt595-evaluation-kit:MIMXRT595-EVK) Connected to an external MIPI display**

**Setup NXP SDK:**

1. Build a NXP SDK
2. Download SDK
3. Extract SDK's zip file to //environment/SDK_2_12_1_EVK-MIMXRT595

**Compile:**

```sh
gn gen out --export-compile-commands --args="
  pw_MIMXRT595_EVK_SDK=\"//environment/SDK_2_12_1_EVK-MIMXRT595\"
  pw_target_mimxrt595_evk_MANIFEST=\"//environment/SDK_2_12_1_EVK-MIMXRT595/EVK-MIMXRT595_manifest_v3_10.xml\"
  pw_third_party_mcuxpresso_SDK=\"//targets/mimxrt595_evk:mimxrt595_sdk\"
"

ninja -C out
```

**Flash the MIMXRT595-EVK:**

Follow the instructions to flash the MIMXRT595-EVK with the SEGGER J-Link
firmware and using `arm-none-eabi-gdb` at
https://pigweed.dev/targets/mimxrt595_evk/target_docs.html#running-and-debugging.

#### Teensy 4.1 ####

https://www.pjrc.com/teensy/loader_cli.html

```bash
brew install teensy_loader_cli
```

```sh
OBJCOPY=/Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avr-objcopy
INFILE=out/arduino_debug/obj/applications/terminal_display/bin/terminal_demo.elf
OUTFILE=foo.hex
$OBJCOPY -O ihex -R .eeprom -R .fuse -R .lock -R .signature $INFILE $OUTFILE
teensy_loader_cli --mcu=TEENSY41 -w -v $OUTFILE
```
