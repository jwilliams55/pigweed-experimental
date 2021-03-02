# Unit Tests & Strings

|||---|||

*** aside
#### [00: <br/> Setup](/workshop/README.md)

`Intro + setup.`
***

*** aside
#### [01: <br/> Blinky](/workshop/01-blinky/README.md)

`Getting to blinky.`
***

*** promo
#### [02: <br/> Testing](/workshop/02-string-functions/README.md)

`Writing tests.`
***

*** aside
#### [03: <br/> RPC](/workshop/03-rpc/README.md)

`Calling RPCs.`
***

*** aside
#### [04: <br/> KVS](/workshop/04-kvs/README.md)

`Key Value Store.`
***

*** aside
#### [05: <br/> FactoryTest](/workshop/05-factory-test/README.md)

`Testing in the factory.`
***

|||---|||

[TOC]

## Build and Flash

Instructions are the same as flashing [blinky](/workshop/01-blinky/README.md)
but passing in a different `.elf`.

1. Run the compile with `pw watch out` or `ninja -C out`.

1. Flash the test `.elf`

   ```sh
   arduino_unit_test_runner --config out/arduino_debug/gen/arduino_builder_config.json --upload-tool teensyloader --verbose --flash-only out/arduino_debug/obj/workshop/02-string-functions/bin/string_demo.elf
   ```

1. Tail the output with `miniterm`, (use `Ctrl-]` to quit).

   ```sh
   python -m serial.tools.miniterm --raw - 115200
   ```

   You should see something like:

   ```text
   $ python -m serial.tools.miniterm --raw - 115200
   --- Available ports:
   ---  1: /dev/ttyACM0         'USB Serial'
   --- Enter port index or full name: 1
   --- Miniterm on /dev/ttyACM0  115200,8,N,1 ---
   --- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
   INF  Teensy Time: RTC has set the system time.
   INF  Offs.  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  Text
   INF  0000: 53 75 70 65 72 20 53 69 6d 70 6c 65 20 54 69 6d  Super Simple Tim
   INF  0010: 65 20 4c 6f 67 67 69 6e 67 21 00                 e Logging!.
   INF  2020-11-04 14:35:35
   INF  2020-11-04 14:35:36
   INF  2020-11-04 14:35:37
   INF  2020-11-04 14:35:38
   INF  2020-11-04 14:35:39

   --- exit ---
   ```

   All tests are set to use plain text logging. This is specified by the
   `pw_log_BACKEND` variable in the `target_toolchain.gni` files. For example
   the `arduino_debug_tests` toolchain in
   [`//targets/arduino/target_toolchains.gni`](/targets/arduino/target_toolchains.gni)
   defines: `pw_log_BACKEND = "$dir_pw_log_basic"`

## Exercise

**Goal:** Write a test for the
[`GetStatusString()`](/workshop/02-string-functions/system_status.cc#9)
function.

It can be added to
[/workshop/02-string-functions/system_status_test.cc](/workshop/02-string-functions/system_status_test.cc).

*** promo
- Refer to the [gTest string comparison
  documentation](https://github.com/google/googletest/blob/master/googletest/docs/primer.md#string-comparison)
  for how to check the contents of strings.
- Note that [gMock
  matchers](https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#more-string-assertions)
  are not supported in Pigweed yet.
***

## Runing Tests

### Run a Single Test Manually

After compiling use `arduino_unit_test_runner` to flash and check test output.

   **Teensy**

   ```sh
   arduino_unit_test_runner --config out/arduino_debug/gen/arduino_builder_config.json --upload-tool teensyloader --verbose out/arduino_debug/obj/workshop/02-string-functions/test/system_status_test.elf
   ```

   **stm32f429i_disc1**

   ```sh
   stm32f429i_disc1_unit_test_runner --verbose out/stm32f429i_disc1_debug/obj/workshop/02-string-functions/test/system_status_test.elf
   ```


### Using the Test Server.

1. Make sure the `pw_arduino_use_test_server=true` build arg is set.

1. Start the unit test server for your board.

   **Teensy**

   ```sh
   arduino_test_server --verbose --config-file ./out/arduino_debug/gen/arduino_builder_config.json
   ```

   **stm32f429i_disc1**

   ```sh
   stm32f429i_disc1_test_server --verbose
   ```

1. In a separate terminal start pw watch.

   ```sh
   pw watch out
   ```
