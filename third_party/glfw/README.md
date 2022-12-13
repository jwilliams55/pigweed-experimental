# GLFW Library

This folder holds the GN build file for the [GLFW](https://www.glfw.org/)
library. The build file requires the `dir_pw_third_party_glfw` be set.
This can be done as so:

```sh
gn gen out --export-compile-commands --args="
  dir_pw_third_party_glfw=\"//environment/packages/glfw\"
"
ninja -C out
```
