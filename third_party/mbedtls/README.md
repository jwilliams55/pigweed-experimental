# Mbedtls Library

The folder hosts mbedtls third party library. The source code is in `src`
folder. A build script `BUILD.gn` is provided and defines a library target for
baremtal use. A number of features are disabled. See `config_baremetal_exp.h`
for more detail. To use the target, add
`//third_party/mbedtls:mbedtls_lib_baremetal` to the dependency list.

`sources.gni` contains the list of source files for building. It is generated
by `generate_sources_gni.py` based on current version of the source code. If
source version bumps up, it may be necessary to re-run the script to keep the
file in sync. The longer term plan is to set up auto-roll and update the file
as part of the rolling process.
