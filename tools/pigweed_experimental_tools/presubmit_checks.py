#!/usr/bin/env python3
# Copyright 2021 The Pigweed Authors
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
"""Example presubmit check script."""

import argparse
import logging
import os
from pathlib import Path
import re
import sys
from typing import Callable

try:
    import pw_cli.log
except ImportError:
    print('ERROR: Activate the environment before running presubmits!',
          file=sys.stderr)
    sys.exit(2)

import pw_presubmit
from pw_presubmit import (
    build,
    cli,
    cpp_checks,
    format_code,
    git_repo,
    inclusive_language,
    install_hook,
    keep_sorted,
    python_checks,
    PresubmitContext,
)

_LOG = logging.getLogger(__name__)

# Set up variables for key project paths.
try:
    PROJECT_ROOT = Path(os.environ['PIGWEED_EXPERIMENTAL_ROOT'])
except KeyError:
    print(
        'ERROR: The presubmit checks must be run in the sample project\'s root'
        ' directory',
        file=sys.stderr)
    sys.exit(2)

PIGWEED_ROOT = PROJECT_ROOT / 'third_party' / 'pigweed'
REPOS = (
    PROJECT_ROOT,
    PIGWEED_ROOT,
    PROJECT_ROOT / 'third_party' / 'nanopb',
)

# Rerun the build if files with these extensions change.
_BUILD_EXTENSIONS = frozenset(
    ['.rst', '.gn', '.gni', *format_code.C_FORMAT.extensions])


#
# Presubmit checks
#
def default_build(ctx: PresubmitContext):
    """Creates a default build."""
    build.gn_gen(ctx)
    build.ninja(ctx)


def _package_root_arg(name: str) -> Callable[[PresubmitContext], str]:
    def _format(ctx: PresubmitContext) -> str:
        return '"{}"'.format(ctx.package_root / name)

    return _format


teensy_build = build.GnGenNinja(
    name='teensy_build',
    packages=('teensy', ),
    gn_args=dict(
        pw_arduino_build_CORE_PATH=lambda ctx: '"{}"'.format(
            str(ctx.package_root)),
        pw_arduino_build_CORE_NAME='"teensy"',
        pw_arduino_build_PACKAGE_NAME='"avr/1.58.1"',
        pw_arduino_build_BOARD='"teensy41"',
        pw_arduino_build_MENU_OPTIONS=(
            '["menu.usb.serial", "menu.keys.en-us", "menu.opt.o2std"]'),
    ),
)

pico_build = build.GnGenNinja(
    name='pico_build',
    packages=('pico_sdk', ),
    gn_args=dict(
        PICO_SRC_DIR=_package_root_arg('pico_sdk'),
        dir_pw_third_party_freertos='"//third_party/freertos/Source"',
    ),
)

stm32cube_f4_build = build.GnGenNinja(
    name='stm32cube_f4_build',
    packages=('stm32cube_f4', ),
    gn_args=dict(
        dir_pw_third_party_stm32cube_f4=_package_root_arg('stm32cube_f4'),
        dir_pw_third_party_freertos='"//third_party/freertos/Source"',
    ),
)

stm32cube_f7_build = build.GnGenNinja(
    name='stm32cube_f7_build',
    packages=('stm32cube_f7', ),
    gn_args=dict(
        dir_pw_third_party_stm32cube_f7=_package_root_arg('stm32cube_f7'),
        dir_pw_third_party_freertos='"//third_party/freertos/Source"',
    ),
)

mimxrt595_evk_build = build.GnGenNinja(
    name='mimxrt595_evk_build',
    gn_args=dict(
        dir_pw_third_party_freertos='"//third_party/freertos/Source"',
        pw_MIMXRT595_EVK_SDK=_package_root_arg("SDK_2_12_1_EVK-MIMXRT595"),
        pw_target_mimxrt595_evk_MANIFEST=_package_root_arg(
            "SDK_2_12_1_EVK-MIMXRT595/EVK-MIMXRT595_manifest_v3_10.xml"),
        pw_third_party_mcuxpresso_SDK="//targets/mimxrt595_evk:mimxrt595_sdk",
    ),
)

pw_graphics_host = build.GnGenNinja(
    name='pw_graphics_host',
    packages=('imgui', 'glfw'),
    gn_args=dict(
        dir_pw_third_party_glfw=_package_root_arg('glfw'),
        dir_pw_third_party_imgui=_package_root_arg('imgui'),
    ),
)


def check_for_git_changes(_: PresubmitContext):
    """Checks that repositories have all changes commited."""
    checked_repos = (PIGWEED_ROOT, *REPOS)
    changes = [r for r in checked_repos if git_repo.has_uncommitted_changes(r)]
    for repo in changes:
        _LOG.error('There are uncommitted changes in the %s repo!', repo.name)
    if changes:
        _LOG.warning(
            'Commit or stash pending changes before running the presubmit.')
        raise pw_presubmit.PresubmitFailure


# Avoid running some checks on certain paths.
PATH_EXCLUSIONS = (
    re.compile(r'^external/'),
    re.compile(r'^third_party/'),
    re.compile(r'^vendor/'),
)

#
# Presubmit check programs
#
OTHER_CHECKS = (
    build.gn_gen_check,
    inclusive_language.presubmit_check.with_filter(exclude=PATH_EXCLUSIONS),
    pw_graphics_host,
    mimxrt595_evk_build,
    teensy_build,
    stm32cube_f7_build,
)

QUICK = (
    # List some presubmit checks to run
    default_build,
    # Use the upstream formatting checks, with custom path filters applied.
    format_code.presubmit_checks(),
)

LINTFORMAT = (
    # keep-sorted: start
    cpp_checks.pragma_once,
    format_code.presubmit_checks(),
    keep_sorted.presubmit_check,
    python_checks.gn_python_lint,
    # keep-sorted: end
)

FULL = (
    cpp_checks.pragma_once,
    QUICK,  # Add all checks from the 'quick' program
    # Use the upstream Python checks, with custom path filters applied.
    python_checks.gn_python_check,
    pico_build,
    stm32cube_f4_build,
)

CI_CQ = (default_build, pico_build, stm32cube_f4_build)

PROGRAMS = pw_presubmit.Programs(
    # keep-sorted: start
    ci_cq=CI_CQ,
    full=FULL,
    lintformat=LINTFORMAT,
    other_checks=OTHER_CHECKS,
    quick=QUICK,
    # keep-sorted: end
)


def run(install: bool, exclude: list, **presubmit_args) -> int:
    """Process the --install argument then invoke pw_presubmit."""

    # Install the presubmit Git pre-push hook, if requested.
    if install:
        install_hook.install_git_hook(
            'pre-push',
            [
                'python',
                '-m',
                'pigweed_experimental_tools.presubmit_checks',
                '--base',
                'origin/main..HEAD',
                '--program',
                'quick',
            ],
        )
        return 0

    exclude.extend(PATH_EXCLUSIONS)
    return cli.run(root=PROJECT_ROOT, exclude=exclude, **presubmit_args)


def main() -> int:
    """Run the presubmit checks for this repository."""
    parser = argparse.ArgumentParser(description=__doc__)
    cli.add_arguments(parser, PROGRAMS, 'quick')

    # Define an option for installing a Git pre-push hook for this script.
    parser.add_argument(
        '--install',
        action='store_true',
        help='Install the presubmit as a Git pre-push hook and exit.')

    return run(**vars(parser.parse_args()))


if __name__ == '__main__':
    pw_cli.log.install(logging.INFO)
    sys.exit(main())
