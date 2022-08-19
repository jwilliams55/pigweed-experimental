# Copyright 2022 The Pigweed Authors
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
"""Install and check status of Glfw."""

from pathlib import Path
import platform
from typing import Sequence

from pw_arduino_build import file_operations
import pw_package.git_repo
import pw_package.package_manager


class Glfw(pw_package.package_manager.Package):
    """Install and check status of Glfw."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, name='glfw', **kwargs)

    def status(self, path: Path) -> bool:
        return path.is_dir()

    def install(self, path: Path) -> None:
        if self.status(path):
            return

        path.mkdir(parents=True, exist_ok=True)

        downloads = {
            'Linux': {
                'url': ('https://github.com/glfw/glfw/releases/download/3.3.8/'
                        'glfw-3.3.8.zip'),
                'sha256sum': ('4d025083cc4a3dd1f91ab9b9ba4f5807'
                              '193823e565a5bcf4be202669d9911ea6'),
            },
            'Darwin': {
                'url': ('https://github.com/glfw/glfw/releases/download/3.3.8/'
                        'glfw-3.3.8.bin.MACOS.zip'),
                'sha256sum': ('dc1fc3d3e7763b9de66f7cbb86c4ba3a'
                              '82118441a15f64045a61cfcdedda88d2'),
            },
            'Windows': {
                'url': ('https://github.com/glfw/glfw/releases/download/3.3.8/'
                        'glfw-3.3.8.bin.WIN64.zip'),
                'sha256sum': ('7851c068b63c3cebf11a3b52c9e7dbdb'
                              '6159afe32666b0aad268e4a258a9bdd1'),
            }
        }

        host_os = platform.system()
        url = downloads[host_os]['url']
        sha256sum = downloads[host_os]['sha256sum']

        glfw_zip = file_operations.download_to_cache(
            url=url, expected_sha256sum=sha256sum, cache_directory=path)

        _extracted_files = file_operations.extract_archive(
            glfw_zip,
            dest_dir=str(path),
            cache_dir=str(path),
            remove_single_toplevel_folder=True)

    def info(self, path: Path) -> Sequence[str]:
        return (
            f'{self.name} installed in: {path}',
            '',
            "Enable by running 'gn args out' and adding this line:",
            f'  dir_pw_third_party_glfw = "{path}"',
            '',
            'NOTE: Installing glfw binaries on Linux is not supported.',
            '      Please install using your package manager.',
            '',
            'Ubuntu/Debian:',
            '  sudo apt install libglfw3-dev libglfw3',
            'Arch Linux',
            '  sudo pacman -S glfw-x11',
        )


pw_package.package_manager.register(Glfw)
