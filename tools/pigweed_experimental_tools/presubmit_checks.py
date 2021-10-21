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
    build.gn_gen(PROJECT_ROOT, ctx.output_dir)
    build.ninja(ctx.output_dir)


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
    inclusive_language.inclusive_language.with_filter(exclude=PATH_EXCLUSIONS),
)

QUICK = (
    # List some presubmit checks to run
    default_build,
    # Use the upstream formatting checks, with custom path filters applied.
    format_code.presubmit_checks(exclude=PATH_EXCLUSIONS),
)

LINTFORMAT = (
    # Use the upstream formatting checks, with custom path filters applied.
    format_code.presubmit_checks(exclude=PATH_EXCLUSIONS),
    python_checks.gn_python_lint.with_filter(exclude=PATH_EXCLUSIONS),
    cpp_checks.pragma_once.with_filter(endswith='.h', exclude=PATH_EXCLUSIONS),
)

FULL = (
    cpp_checks.pragma_once.with_filter(endswith='.h', exclude=PATH_EXCLUSIONS),
    QUICK,  # Add all checks from the 'quick' program
    # Use the upstream Python checks, with custom path filters applied.
    python_checks.gn_python_check.with_filter(exclude=PATH_EXCLUSIONS),
)

PROGRAMS = pw_presubmit.Programs(
    quick=QUICK,
    full=FULL,
    lintformat=LINTFORMAT,
    other_checks=OTHER_CHECKS,
)


def run(install: bool, **presubmit_args) -> int:
    """Process the --install argument then invoke pw_presubmit."""

    # Install the presubmit Git pre-push hook, if requested.
    if install:
        install_hook.install_hook(__file__, 'pre-push', ['--base', 'HEAD~'],
                                  git_repo.root())
        return 0

    return cli.run(root=PROJECT_ROOT, **presubmit_args)


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
