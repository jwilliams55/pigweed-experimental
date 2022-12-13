# ImGui Library

This folder holds the GN build file for the [ImGui](https://www.dearimgui.org/)
library. The build file requires the `dir_pw_third_party_imgui` be set.
This can be done as so:

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_imgui=\"//environment/packages/imgui\"
"
ninja -C out
```
