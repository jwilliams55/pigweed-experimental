##Building
In order to build this target, the submodules in `//third_party/stm32cubef4`
need to be checked out and the following flag needs to be added to your
gn args (gn args out)

```
pw_third_party_stm32cubef4_enabled = "yes"
```

##Flashing

Images can be flashed using the same scripts as the in-tree variant.

This command can be used to flash the blinky example:

```
openocd -s ${PW_PIGWEED_CIPD_INSTALL_DIR}/share/openocd/scripts -f ${PW_ROOT}/targets/stm32f429i-disc1/py/stm32f429i_disc1_utils/openocd_stm32f4xx.cfg -c "program out/stm32f429i_disc1_stm32cube_debug/obj/applications/blinky/bin/blinky.elf reset exit"
```
