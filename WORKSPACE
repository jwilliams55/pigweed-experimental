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
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

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
    name = "rules_proto",
    sha256 = "dc3fb206a2cb3441b485eb1e423165b231235a1ea9b031b4433cf7bc1fa460dd",
    strip_prefix = "rules_proto-5.3.0-21.7",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/5.3.0-21.7.tar.gz",
    ],
)

http_archive(
    name = "rules_python",
    sha256 = "9acc0944c94adb23fba1c9988b48768b1bacc6583b52a2586895c5b7491e2e31",
    strip_prefix = "rules_python-0.27.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.27.0/rules_python-0.27.0.tar.gz",
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

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
)

cipd_client_repository()

load("@pigweed//pw_toolchain:register_toolchains.bzl", "register_pigweed_cxx_toolchains")

register_pigweed_cxx_toolchains()

git_repository(
  name = "pico-sdk",
  remote = "https://github.com/armandomontanez/pico-sdk.git",
  commit = "07890a2af5d76a68a28d0c286d8c4a6611b904f0",
)

http_archive(
    name = "arm_gcc_linux-x86_64",
    url = "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz",
    strip_prefix = "arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-eabi",
    build_file = "@pw_toolchain//build_external:gcc_arm_none_eabi.BUILD",
    sha256 = "6cd1bbc1d9ae57312bcd169ae283153a9572bd6a8e4eeae2fedfbc33b115fdbb",
)
