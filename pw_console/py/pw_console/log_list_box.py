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

import logging
import pprint

import urwid

_LOG = logging.getLogger(__name__)

_pretty_print = pprint.PrettyPrinter(indent=1, width=120).pprint
_pretty_format = pprint.PrettyPrinter(indent=1, width=120).pformat


class LogListBox(urwid.ListBox):
    def __init__(self, node, parent_app=None):
        self.parent_app = parent_app
        super().__init__(node)
        # insert an extra AttrWrap for our own use
        # self._w = urwid.AttrWrap(self._w, None)
        # self.flagged = False
        # self.update_w()

    def get_visible_widget_count(self, size):
        visible_widget_count = 1
        unused_middle, top, bottom = self.calculate_visible(size)
        if top and bottom:
            visible_widget_count = len(top[1]) + len(bottom[1])
        return visible_widget_count

    def scroll_page_down(self, size, factor=1.0):
        new_focus_position = min([
            self.focus_position +
            round(self.get_visible_widget_count(size) * factor),
            len(self.body) - 1
        ])
        self.set_focus(new_focus_position, 'below')
        self.set_focus_valign(('relative', 100))

    def scroll_page_up(self, size, factor=1.0):
        new_focus_position = max([
            self.focus_position -
            round(self.get_visible_widget_count(size) * factor), 0
        ])
        self.set_focus(new_focus_position, 'above')
        self.set_focus_valign(('relative', 0))

    def keypress(self, size, key):
        _LOG.debug(
            _pretty_format({
                "function":
                type(self).__name__ + '.keypress',
                "key":
                key,
                "focus":
                self.focus if len(self.body) > 0 else None,
                "focus_postition":
                self.focus_position if len(self.body) > 0 else None
            }))

        # intercept keypresses, return key if unhandled
        if self.parent_app.key_bindings.is_bound_to(key, 'down'):
            new_focus_position = min(
                [self.focus_position + 1,
                 len(self.body) - 1])
            self.set_focus(new_focus_position, 'below')
            self.set_focus_valign(('relative', 50))

        elif self.parent_app.key_bindings.is_bound_to(key, 'up'):
            new_focus_position = max([self.focus_position - 1, 0])
            self.set_focus(new_focus_position, 'above')
            self.set_focus_valign(('relative', 50))

        elif self.parent_app.key_bindings.is_bound_to(key, 'scroll-page-down'):
            self.scroll_page_down(size)

        elif self.parent_app.key_bindings.is_bound_to(key, 'scroll-page-up'):
            self.scroll_page_up(size)

        else:
            # if unhandled pass key along to superclass
            key = super().keypress(size, key)
            # if still unhandled
            if key:
                return key

        return

    def mouse_event(self, size, event, button, col, row, focus):
        _LOG.debug(
            _pretty_format({
                "function":
                type(self).__name__ + '.mouse_event',
                "event":
                event,
                "size":
                size,
                "button":
                button,
                "focus":
                self.focus if len(self.body) > 0 else None,
                "focus selectable?":
                self.focus.selectable() if len(self.body) > 0 else None,
                "focus_postition":
                self.focus_position if len(self.body) > 0 else None
            }))

        # Mouse Scrolling
        if len(self.body) > 0 and event == "mouse press" and button == 4:
            self.scroll_page_down(size, factor=0.5)

        elif len(self.body) > 0 and event == "mouse press" and button == 5:
            self.scroll_page_up(size, factor=0.5)

        else:
            # Call parent class (urwid.ListBox) mouse_event
            # to handle button 1 (left) click.
            return super().mouse_event(size, event, button, col, row, focus)

        return True  # event was handled
