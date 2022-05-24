#!/usr/bin/env python3
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
"""PNG to rgb565 cc file converter."""

import argparse
import logging
import math
from pathlib import Path

from PIL import Image  # type: ignore
from jinja2 import Environment, FileSystemLoader, make_logging_undefined

jinja_env = Environment(
    # Load templates automatically from pw_console/templates
    loader=FileSystemLoader(Path(__file__).parent / 'templates'),
    # Raise errors if variables are undefined in templates
    undefined=make_logging_undefined(logger=logging.getLogger(__package__), ),
    # Trim whitespace in templates
    trim_blocks=True,
    lstrip_blocks=True,
)


def _arg_parser():
    """Setup argparse."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('IMAGE', type=Path, help="Input image. ")
    parser.add_argument('-W',
                        '--sprite-width',
                        type=int,
                        required=True,
                        help='Sprite width.')
    parser.add_argument('-H',
                        '--sprite-height',
                        type=int,
                        required=True,
                        help='Sprite height.')
    parser.add_argument('--output-mode',
                        default='rgb565',
                        choices=['rgb565', 'font'],
                        help='')
    parser.add_argument(
        '--transparent-color',
        default='255,0,255',
        help='Comma separated r,g,b values to use as transparent.')
    return parser.parse_args()


def rgb8888to565(rgba8888):
    red = ((rgba8888 & 0xFF0000) >> 16)
    green = ((rgba8888 & 0xFF00) >> 8)
    blue = ((rgba8888 & 0xFF))
    return rgb565(red, green, blue)


def rgb565(red, green, blue) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3)


def render_rgb565_header(
    alpha_composite: Image,
    tile_width: int,
    tile_height: int,
    sprite_width: int,
    sprite_height: int,
) -> list:
    sprite_data: list[list[str]] = []
    for theight in range(0, tile_height):
        for twidth in range(0, tile_width):
            sprite_data.append([])
            for currenty in range(0, sprite_height):
                for currentx in range(0, sprite_width):
                    globalx = twidth * sprite_width + currentx
                    globaly = theight * sprite_height + currenty
                    pix = alpha_composite.getpixel((globalx, globaly))

                    hex_rgb565 = rgb565(pix[0], pix[1], pix[2])
                    hex_r = f"{pix[0]:02x}"
                    hex_g = f"{pix[1]:02x}"
                    hex_b = f"{pix[2]:02x}"
                    sprite_data[-1].append(
                        f"{hex_rgb565:#04x},  // #{hex_r}{hex_g}{hex_b}\n")
    return sprite_data


def render_font_header(
    alpha_composite: Image,
    tile_width: int,
    tile_height: int,
    sprite_width: int,
    sprite_height: int,
) -> list:
    sprite_data: list[list[str]] = []
    for theight in range(0, tile_height):
        for twidth in range(0, tile_width):
            sprite_data.append([])
            for currenty in range(0, sprite_height):
                line = '0b'
                for currentx in range(0, sprite_width):
                    globalx = twidth * sprite_width + currentx
                    globaly = theight * sprite_height + currenty
                    pix = alpha_composite.getpixel((globalx, globaly))
                    if pix[0] + pix[1] + pix[2] == 0:
                        line += '0'
                    else:
                        line += '1'
                line += ',\n'
                sprite_data[-1].append(line)
    return sprite_data


def main() -> None:
    """Main."""
    args = _arg_parser()

    image_path = args.IMAGE
    file_name = image_path.stem.lower().replace(' ', '_')
    sprite_width = args.sprite_width
    sprite_height = args.sprite_height
    if not image_path.is_file():
        raise FileNotFoundError

    transparent_color = (255, 0, 255)
    if args.transparent_color:
        rgb = args.transparent_color.split(',')
        assert len(rgb) == 3
        transparent_color = (int(rgb[0]), int(rgb[1]), int(rgb[2]))

    out_path = Path(image_path.stem + '.h')

    img = Image.open(image_path).convert('RGBA')
    background = Image.new('RGBA', img.size, transparent_color)
    alpha_composite = Image.alpha_composite(background, img)
    transparent_color_int = rgb565(*transparent_color)

    image_width = img.size[0]
    image_height = img.size[1]
    tile_width = math.floor(image_width / sprite_width)
    tile_height = math.floor(image_height / sprite_height)
    tile_width = max(tile_width, 1)
    tile_height = max(tile_height, 1)

    if args.output_mode == 'rgb565':
        sprite_data = render_rgb565_header(alpha_composite, tile_width,
                                           tile_height, sprite_width,
                                           sprite_height)
    elif args.output_mode == 'font':
        sprite_data = render_font_header(alpha_composite, tile_width,
                                         tile_height, sprite_width,
                                         sprite_height)

    template = jinja_env.get_template(args.output_mode + '.jinja')
    out_path.write_text(
        template.render(
            file_name=file_name,
            sprite_data=sprite_data,
            sprite_width=sprite_width,
            sprite_height=sprite_height,
            transparent_color=f"{transparent_color_int:#04x}",
            count=len(sprite_data),
        ))


if __name__ == '__main__':
    main()
