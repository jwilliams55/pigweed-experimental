# Copyright 2023 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "pigweed",
    path = "third_party/pigweed",
)

local_repository(
    name = "pw_toolchain",
    path = "third_party/pigweed/pw_toolchain_bazel",
)

http_archive(
    name = "platforms",
    sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
        "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
    ],
)

http_archive(
    name = "bazel_skylib",  # 2022-09-01
    sha256 = "4756ab3ec46d94d99e5ed685d2d24aece484015e45af303eb3a11cab3cdc2e71",
    strip_prefix = "bazel-skylib-1.3.0",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/refs/tags/1.3.0.zip"],
)

http_archive(
    name = "com_google_protobuf",
    sha256 = "c6003e1d2e7fefa78a3039f19f383b4f3a61e81be8c19356f85b6461998ad3db",
    strip_prefix = "protobuf-3.17.3",
    url = "https://github.com/protocolbuffers/protobuf/archive/v3.17.3.tar.gz",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

# Get ready to grab CIPD dependencies. For this minimal example, the only
# dependencies will be the toolchains and OpenOCD (used for flashing).
load(
    "@pigweed//pw_env_setup/bazel/cipd_setup:cipd_rules.bzl",
    "cipd_client_repository",
    "cipd_repository",
    "pigweed_deps",
)

pigweed_deps()

load("@cipd_deps//:cipd_init.bzl", "cipd_init")

cipd_init()

cipd_client_repository()

# Setup xcode on mac.
load("@pw_toolchain//features/macos:generate_xcode_repository.bzl", "pw_xcode_command_line_tools_repository")

pw_xcode_command_line_tools_repository()

# Fetch llvm toolchain.
cipd_repository(
    name = "llvm_toolchain",
    path = "fuchsia/third_party/clang/${os}-${arch}",
    tag = "git_revision:8475d0a2b853f6184948b428ec679edf84ed2688",
)

# Fetch linux sysroot for host builds.
cipd_repository(
    name = "linux_sysroot",
    path = "fuchsia/third_party/sysroot/linux",
    tag = "git_revision:d342388843734b6c5c50fb7e18cd3a76476b93aa",
)

# Fetch gcc-arm-none-eabi toolchain.
cipd_repository(
    name = "gcc_arm_none_eabi_toolchain",
    path = "fuchsia/third_party/armgcc/${os}-${arch}",
    tag = "version:2@12.2.mpacbti-rel1.1",
)

# Use upstream Pigweed toolchain configurations.
register_toolchains(
    "@pigweed//pw_toolchain/host_clang:host_cc_toolchain_linux",
    "@pigweed//pw_toolchain/host_clang:host_cc_toolchain_macos",
    "@pigweed//pw_toolchain/arm_gcc:arm_gcc_cc_toolchain_cortex-m4+nofp",
)