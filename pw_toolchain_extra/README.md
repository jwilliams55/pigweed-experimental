# pw_toolchain_extra

[TOC]

## Overview
This module provides experimental toolchain-related features. Some of these may
make their way into Pigweed, some are illustrative examples.

## GN pw_toolchain and pw_cortex_m_gcc_toolchain templates
These templates imagine a slightly different API for instantiating toolchains.
Rather than toolchains being pre-defined scopes that you can import and extend
or override, `pw_cortex_m_gcc_toolchain` illustrates a configurable toolchain
that puts more control into the hands of the user while also being more legible
at-a-glance.

`pw_cortex_m_gcc_toolchain` is built on top of `pw_toolchain`. The naming of the
`pw_toolchain` template is intentionally similar to `generate_toolchain`, and
this is for two reasons:

1. `generate_toolchain` should have been named `pw_generate_toolchain` from the
   Beginning. Namespacing templates behind `pw_*` is a well-established pattern
   that unfortunately wasn't applied to the `generate_toolchain` template during
   its inception. `pw_toolchain` takes the verb out of the name, and prefixes
   with `pw_*` to match the established naming patterns.
2. `pw_toolchain` extends upon and improves `generate_toolchain` with the intent
   to eventually replace it. This naming difference provides a migration path.

The `pw_toolchain` template also makes it easier to stand up RBE for a given
toolchain by parameterizing the required flags.

As the GN build will likely be turned down in the distant future, it's unlikely
these templates will make their way into the core Pigweed repository. It's left
here as a proof-of-concept for the direction Pigweed might have gone
