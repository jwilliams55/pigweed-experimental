# Copyright 2020 The Pigweed Authors
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
"""ConsoleApp control class."""

from dataclasses import dataclass, field
from typing import List
# import asyncio
import logging
import pprint

from prompt_toolkit.styles import Style
from prompt_toolkit.application import Application
from prompt_toolkit.buffer import Buffer
from prompt_toolkit.key_binding import KeyBindings
from prompt_toolkit.key_binding.bindings.focus import focus_next, focus_previous
from prompt_toolkit.layout.controls import BufferControl, FormattedTextControl
from prompt_toolkit.layout.dimension import Dimension
from prompt_toolkit.layout.layout import Layout
# from prompt_toolkit.mouse_events import MouseEvent, MouseEventType
from prompt_toolkit.layout.containers import (
    HSplit,
    VSplit,
    Window,
    WindowAlign,
    HorizontalAlign,
)

from pw_tokenizer.tokens import Database

_LOG = logging.getLogger(__name__)

_pretty_print = pprint.PrettyPrinter(indent=1, width=120).pprint
_pretty_format = pprint.PrettyPrinter(indent=1, width=120).pformat

style = Style.from_dict({
    "top_toolbar":
    "bg:#3e4452 #abb2bf",
    "top_toolbar_colored_text":
    "bg:#282c34 #c678dd",
    "top_toolbar_colored_background":
    "bg:#c678dd #282c34",
    "bottom_toolbar":
    "bg:#3e4452 #abb2bf",
    "bottom_toolbar_colored_text":
    "bg:#282c34 #61afef",
    "bottom_toolbar_colored_background":
    "bg:#61afef #282c34",
})

bindings: KeyBindings = KeyBindings()


@bindings.add("c-q")
@bindings.add("c-d")
def exit_(event):
    """
    Pressing Ctrl-Q will exit the user interface.

    Setting a return value means: quit the event loop that drives the user
    interface and return this value from the `Application.run()` call.
    """
    # asyncio.get_event_loop().stop()
    event.app.exit()


@bindings.add("s-tab")
@bindings.add("c-right")
@bindings.add("c-down")
def app_focus_next(event):
    """Rotate focus to the next widget."""
    _LOG.debug(_pretty_format(event))
    focus_next(event)


@bindings.add("c-left")
@bindings.add("c-up")
def app_focus_previous(event):
    """Rotate focus to the previous widget."""
    focus_previous(event)


@bindings.add("f1")
def app_resize_split(event):
    """Resize split."""
    current_window = event.app.layout.current_window
    current_window.height.weight += 1
    _LOG.debug(_pretty_format(current_window.height))


def _create_top_toolbar() -> VSplit:
    """Create the global top toolbar."""
    return VSplit([
        Window(content=FormattedTextControl(
            [("class:top_toolbar_colored_background", " LeftText ")]),
               align=WindowAlign.LEFT,
               dont_extend_width=True),
        Window(content=FormattedTextControl(
            [("class:top_toolbar", " center text ")]),
               align=WindowAlign.LEFT,
               dont_extend_width=False),
        Window(content=FormattedTextControl(
            [("class:top_toolbar_colored_text", " right-text ")]),
               align=WindowAlign.RIGHT,
               dont_extend_width=True),
    ],
                  height=1,
                  style="class:top_toolbar",
                  align=HorizontalAlign.LEFT)


def _create_bottom_toolbar():
    """Create the global bottom toolbar."""
    return VSplit([
        Window(content=FormattedTextControl(
            [("class:bottom_toolbar_colored_background", " Connected ")]),
               align=WindowAlign.LEFT,
               dont_extend_width=True),
        Window(content=FormattedTextControl(
            [("class:bottom_toolbar", " 256 Logs ")]),
               align=WindowAlign.LEFT,
               dont_extend_width=False),
        Window(content=FormattedTextControl(
            [("class:bottom_toolbar_colored_text",
              " file_name=2021-03-05_1454_log.txt ")]),
               align=WindowAlign.RIGHT,
               dont_extend_width=True),
    ],
                  height=1,
                  style="class:bottom_toolbar",
                  align=WindowAlign.LEFT)


@dataclass
class ConsoleApp:
    # pylint: disable=too-many-instance-attributes
    """ConsoleApp class."""

    log_file_name: str
    token_databases: Database
    terminal_args: List[str] = field(default_factory=list)
    # prompt_toolkit
    buffer1: Buffer = Buffer()  # Editable buffer.
    buffer2: Buffer = Buffer()  # Editable buffer.

    buffer1_window: Window = Window(content=BufferControl(buffer=buffer1),
                                    height=Dimension(weight=1),
                                    dont_extend_height=False)

    split_window: Window = Window(height=1, char="-")

    buffer2_window: Window = Window(content=BufferControl(buffer=buffer2),
                                    height=Dimension(weight=1),
                                    dont_extend_height=False)

    async def run(self):
        # pylint: disable=attribute-defined-outside-init
        """Start the prompt_toolkit console UI."""

        self.top_toolbar = _create_top_toolbar()

        self.bottom_toolbar = _create_bottom_toolbar()

        self.root_container: HSplit = HSplit([
            self.top_toolbar,
            self.buffer1_window,
            self.split_window,
            self.buffer2_window,
            self.bottom_toolbar,
        ])

        self.layout: Layout = Layout(self.root_container,
                                     focused_element=self.buffer1_window)
        _LOG.debug(_pretty_format(self))

        # self.buffer1_window.height = 10
        # self.buffer1_window.dont_extend_height = True
        # self.buffer1_window.preferred_height(max_available_height=10)
        # Define application.
        application = Application(layout=self.layout,
                                  full_screen=True,
                                  key_bindings=bindings,
                                  style=style,
                                  mouse_support=True)
        result = await application.run_async()
        print(result)
        # asyncio.get_event_loop().run_until_complete(self.main())
