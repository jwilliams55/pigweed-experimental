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
# pylint: skip-file
# type: ignore
"""LogLineWidget."""

import logging
import urwid

from pw_console.title_line_box import TitleLineBox

_LOG = logging.getLogger(__name__)


class LogLineWidget(urwid.Button):
    def __init__(self,
                 log,
                 key_bindings,
                 colorscheme,
                 parent_ui,
                 editing=False,
                 display_timestamps=False,
                 display_levels=False,
                 wrapping='space',
                 border='no border'):
        super(LogLineWidget, self).__init__("")
        self.log = log
        self.key_bindings = key_bindings
        self.wrapping = wrapping
        self.border = border
        self.colorscheme = colorscheme
        self.parent_ui = parent_ui
        self.editing = editing
        self.display_timestamps = display_timestamps
        self.display_levels = display_levels
        # TODO(tonymd): make click select/focus the log line
        # urwid.connect_signal(self, 'click', callback)
        self.update()

    def selectable(self):
        return True

    def keypress(self, size, key):
        # pp(['LogLineWidget', size, key, self.focus, self.focus_position])
        if self.editing:
            if key in ['down', 'up']:
                return  # don't pass up or down to the ListBox
            elif self.key_bindings.is_bound_to(key, 'save-item'):
                self.save_item()
                return key
            else:
                return self._w.keypress(size, key)
        else:
            if self.key_bindings.is_bound_to(key, 'edit'):
                self.edit_item()
                return key
            else:
                return key

    def update(self):
        if self.parent_ui.searching and self.parent_ui.search_string:
            text = urwid.Text(self.log.highlight_search_matches(),
                              wrap=self.wrapping)
        else:
            if self.border == 'bordered':
                text = urwid.Text(self.log.highlight(), wrap=self.wrapping)
            else:
                text = urwid.Text(self.log.highlight(
                    show_timestamp=self.display_timestamps,
                    show_log_level=self.display_levels),
                                  wrap=self.wrapping)

        if self.border == 'bordered':
            lt = ''
            log_time = [('log_time', self.log.formatted_time())]
            log_level = [('log_level', "INF")]
            bc = 'plain'
            bc = "priority_{0}".format("a")
            text = TitleLineBox(
                text,
                top_left_title=log_time,
                bottom_right_title=log_level,
                border_color=bc,
            )
        self._w = urwid.AttrMap(urwid.AttrMap(text, None, 'selected'), None,
                                self.colorscheme.focus_map)
