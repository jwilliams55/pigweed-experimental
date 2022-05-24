# pw_display_teensy_ili9341

## Setup Instructions

1. Install the Teensyduino core and set the required GN args.

   ```sh
   pw package install teensy
   ```

2. Edit `//targets/arduino/target_toolchains.gni` and set:

   ```
   pw_display_BACKEND = "$dir_pw_display_teensy_ili9341"
   ```
