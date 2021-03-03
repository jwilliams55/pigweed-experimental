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

import time
import collections
import logging
import os
import pprint
import re
import subprocess
from pathlib import Path

import urwid

from pw_console.log_line import LogLine
from pw_console.log_line_widget import LogLineWidget
from pw_console.log_list_box import LogListBox
from pw_console.search_widget import SearchWidget

from pw_tokenizer import tokens
from pw_tokenizer.detokenize import Detokenizer, detokenize_base64

_LOG = logging.getLogger(__name__)

_pretty_print = pprint.PrettyPrinter(indent=1, width=120).pprint
_pretty_format = pprint.PrettyPrinter(indent=1, width=120).pformat


class ViPile(urwid.Pile):
    def __init__(self, key_bindings, widget_list, focus_item=None):
        """Pile with Vi-like navigation."""
        super(ViPile, self).__init__(widget_list, focus_item)

        command_map = urwid.command_map.copy()

        keys = key_bindings.getKeyBinding('up')
        for key in keys:
            command_map[key] = urwid.CURSOR_UP
        keys = key_bindings.getKeyBinding('down')
        for key in keys:
            command_map[key] = urwid.CURSOR_DOWN

        self._command_map = command_map


class ViColumns(urwid.Columns):
    def __init__(self,
                 key_bindings,
                 widget_list,
                 dividechars=0,
                 focus_column=None,
                 min_width=1,
                 box_columns=None):
        super(ViColumns, self).__init__(widget_list, dividechars, focus_column,
                                        min_width, box_columns)
        command_map = urwid.command_map.copy()

        keys = key_bindings.getKeyBinding('right')
        for key in keys:
            command_map[key] = urwid.CURSOR_RIGHT
        keys = key_bindings.getKeyBinding('left')
        for key in keys:
            command_map[key] = urwid.CURSOR_LEFT

        self._command_map = command_map


class ViListBox(urwid.ListBox):
    def __init__(self, key_bindings, *args, **kwargs):
        super(ViListBox, self).__init__(*args, **kwargs)
        command_map = urwid.command_map.copy()

        keys = key_bindings.getKeyBinding('down')
        for key in keys:
            command_map[key] = urwid.CURSOR_DOWN
        keys = key_bindings.getKeyBinding('up')
        for key in keys:
            command_map[key] = urwid.CURSOR_UP

        self._command_map = command_map


_incoming_logs = []


class ConsoleApp:
    def __init__(self,
                 log_file_name,
                 key_bindings,
                 colorscheme,
                 terminal_args=None,
                 token_databases=None):
        self.logs = []
        self.log_file_path = Path(log_file_name)
        self.log_file_proc = None
        self.terminal_args = terminal_args
        self.token_databases = token_databases
        self.detokenizer = None
        if self.token_databases:
            self.detokenizer = Detokenizer(
                tokens.Database.merged(*self.token_databases),
                show_errors=False)

        # urwid widgets
        self.log_header = None
        self.log_footer = None
        self.log_listbox = None
        self.log_frame = None
        self.terminal_frame_header = None
        self.ipython_terminal = None
        self.terminal_frame = None
        self.log_and_terminal_pile = None
        self.view = None
        self.loop = None

        # TODO(tonymd): dont write a new log file until data is read off device.
        # if not self.log_file_path.is_file():
        #     self.log_file_path.touch(exist_ok=True)
        #     with self.log_file_path.open(mode="w") as log_file:
        #         log_file.write(
        #             "Empty log file. Hit ? for help. Mouse click to focus on panes.\n"
        #         )

        self.display_timestamps = True
        self.display_levels = True
        self.follow_new_log_entries = True
        self.wrapping = collections.deque(['clip', 'space'])
        self.border = collections.deque(['no border', 'bordered'])
        self.sorting = collections.deque(
            ["Unsorted", "Descending", "Ascending"])
        self.sorting_display = {
            "Unsorted": "-",
            "Descending": "v",
            "Ascending": "^"
        }

        self.key_bindings = key_bindings

        self.colorscheme = colorscheme
        self.palette = [(key, '', '', '', value['fg'], value['bg'])
                        for key, value in self.colorscheme.colors.items()]

        self.active_projects = []
        self.active_contexts = []

        self.toolbar_is_open = False
        self.help_panel_is_open = False
        self.filter_panel_is_open = False
        self.filtering = False
        self.searching = False
        self.search_string = ''
        self.yanked_text = ''

    def filter_log_list(self):
        self.delete_log_widgets()
        self.filtering = True

    def update_filters(self, new_contexts=[], new_projects=[]):
        if self.active_contexts:
            for c in new_contexts:
                self.active_contexts.append(c)
        if self.active_projects:
            for p in new_projects:
                self.active_projects.append(p)
        self.update_filter_panel()

    def reload_from_file(self):
        self.delete_log_widgets()
        self.load_logs_from_file()
        self.create_log_widgets()
        self.update_header("Reloaded")

    def load_logs_from_file(self):
        with open(self.log_file_path.as_posix(), "r") as log_file:
            self.logs = [LogLine(line) for line in log_file.readlines()]

    def delete_log_widgets(self):
        for i in range(len(self.log_listbox.body) - 1, -1, -1):
            self.log_listbox.body.pop(i)

    def append_new_log_widget(self, log):
        self.log_listbox.body.append(
            LogLineWidget(log,
                          self.key_bindings,
                          self.colorscheme,
                          self,
                          wrapping=self.wrapping[0],
                          border=self.border[0],
                          display_timestamps=self.display_timestamps,
                          display_levels=self.display_levels))

    def create_log_widgets(self):
        for log in self.logs:
            self.append_new_log_widget(log)

    def reload_logs_from_memory(self):
        self.delete_log_widgets()
        self.create_log_widgets()

    def clear_filters(self):
        self.delete_log_widgets()
        self.reload_logs_from_memory()
        self.active_projects = []
        self.active_contexts = []
        self.filtering = False
        self.view.set_focus(0)
        self.update_filters()

    def checkbox_clicked(self, checkbox, state, data):
        if state:
            if data[0] == 'context':
                self.active_contexts.append(data[1])
            else:
                self.active_projects.append(data[1])
        else:
            if data[0] == 'context':
                self.active_contexts.remove(data[1])
            else:
                self.active_projects.remove(data[1])

        if self.active_projects or self.active_contexts:
            self.filter_log_list()
            self.view.set_focus(0)
        else:
            self.clear_filters()

    def visible_lines(self):
        lines = self.loop.screen_size[1] - 1  # minus one for the header
        if self.toolbar_is_open:
            lines -= 1
        if self.searching:
            lines -= 1
        return lines

    def move_selection_top(self):
        self.log_listbox.set_focus(0)

    def move_selection_bottom(self):
        self.log_listbox.set_focus(len(self.log_listbox.body) - 1)

    def toggle_help_panel(self, button=None):
        if self.filter_panel_is_open:
            self.toggle_filter_panel()
        if self.help_panel_is_open:
            self.view.contents.pop()
            self.help_panel_is_open = False
            # set header line to word-wrap contents
            # for header_column in self.log_frame.header.original_widget.contents:
            #     header_column[0].set_wrap_mode('space')
        else:
            self.help_panel = self.create_help_panel()
            self.view.contents.append((self.help_panel,
                                       self.view.options(width_type='weight',
                                                         width_amount=2)))
            self.view.set_focus(1)
            self.help_panel_is_open = True
            # set header line to clip contents
            # for header_column in self.log_frame.header.original_widget.contents:
            #     header_column[0].set_wrap_mode('clip')

    def toggle_sorting(self, button=None):
        self.delete_log_widgets()
        self.sorting.rotate(1)
        if self.sorting[0] == 'Ascending':
            self.logs.sort(key=lambda log: log.raw_line, reverse=False)
        elif self.sorting[0] == 'Descending':
            self.logs.sort(key=lambda log: log.raw_line, reverse=True)
        elif self.sorting[0] == 'Unsorted':
            pass
        self.reload_logs_from_memory()
        self.move_selection_top()
        self.update_header()

    def toggle_filter_panel(self, button=None):
        if self.help_panel_is_open:
            self.toggle_help_panel()
        if self.filter_panel_is_open:
            self.view.contents.pop()
            self.filter_panel_is_open = False
        else:
            self.filter_panel = self.create_filter_panel()
            self.view.contents.append((self.filter_panel,
                                       self.view.options(width_type='weight',
                                                         width_amount=1)))
            self.filter_panel_is_open = True

    def toggle_wrapping(self, checkbox=None, state=None):
        self.wrapping.rotate(1)
        for widget in self.log_listbox.body:
            widget.wrapping = self.wrapping[0]
            widget.update()
        if self.toolbar_is_open:
            self.update_header()

    def toggle_levels(self, checkbox=None, state=None):
        self.display_levels = not self.display_levels
        for widget in self.log_listbox.body:
            widget.display_levels = self.display_levels
            widget.update()
        if self.toolbar_is_open:
            self.update_header()

    def toggle_timestamps(self, checkbox=None, state=None):
        self.display_timestamps = not self.display_timestamps
        for widget in self.log_listbox.body:
            widget.display_timestamps = self.display_timestamps
            widget.update()
        if self.toolbar_is_open:
            self.update_header()

    def toggle_follow(self, checkbox=None, state=None):
        self.follow_new_log_entries = not self.follow_new_log_entries
        if self.toolbar_is_open:
            self.update_header()

    def toggle_border(self, checkbox=None, state=None):
        self.border.rotate(1)
        for widget in self.log_listbox.body:
            widget.border = self.border[0]
            widget.update()
        if self.toolbar_is_open:
            self.update_header()

    def toggle_toolbar(self):
        self.toolbar_is_open = not self.toolbar_is_open
        self.update_header()

    def search_box_updated(self, edit_widget, new_contents):
        self.search_string = new_contents
        self.search_log_list(self.search_string)

    def fuzzy_search(self, search_string):
        search_string = re.escape(search_string)
        ss = []
        substrings = search_string.split("\\")
        for index, substring in enumerate(substrings):
            s = ".*?".join(substring)
            if 0 < index < len(substrings) - 1:
                s += ".*?"
            ss.append(s)
        search_string_regex = '^.*('
        search_string_regex += "\\".join(ss)
        search_string_regex += ').*'

        r = re.compile(search_string_regex, re.IGNORECASE)
        results = []
        for log in self.logs:
            match = r.search(log.raw_line)
            if match:
                log.search_matches = match.groups()
                results.append(log)
        return results

    def search_log_list(self, search_string=""):
        if search_string:
            self.searching = True
            self.delete_log_widgets()
            for log in self.fuzzy_search(search_string):
                self.log_listbox.body.append(
                    LogLineWidget(log,
                                  self.key_bindings,
                                  self.colorscheme,
                                  self,
                                  wrapping=self.wrapping[0],
                                  border=self.border[0]))

    def start_search(self):
        self.searching = True
        self.update_footer()
        self.log_frame.set_focus('footer')

    def finalize_search(self):
        self.search_string = ''
        self.log_frame.set_focus('body')
        for widget in self.log_listbox.body:
            widget.update()

    def clear_search_term(self, button=None):
        self.delete_log_widgets()
        self.searching = False
        self.search_string = ''
        self.update_footer()
        self.reload_logs_from_memory()

    def keystroke(self, input):
        _LOG.debug(
            _pretty_format({
                "function": type(self).__name__ + '.keystroke',
                "key": input,
                "focus": self.view.focus,
                "focus_postition": self.view.focus_position
            }))
        if self.key_bindings.is_bound_to(input, 'quit'):
            raise urwid.ExitMainLoop()
        # Movement
        elif self.key_bindings.is_bound_to(input, 'top'):
            self.move_selection_top()
        elif self.key_bindings.is_bound_to(input, 'bottom'):
            self.move_selection_bottom()
        elif self.key_bindings.is_bound_to(input, 'swap-down'):
            self.swap_down()
        elif self.key_bindings.is_bound_to(input, 'swap-up'):
            self.swap_up()
        elif self.key_bindings.is_bound_to(input, 'change-focus'):
            current_focus = self.log_frame.get_focus()
            if current_focus == 'body':

                if self.filter_panel_is_open and self.toolbar_is_open:

                    if self.view.focus_position == 1:
                        self.view.focus_position = 0
                        self.log_frame.focus_position = 'header'
                    elif self.view.focus_position == 0:
                        self.view.focus_position = 1

                elif self.toolbar_is_open:
                    self.log_frame.focus_position = 'header'

                elif self.filter_panel_is_open:
                    if self.view.focus_position == 1:
                        self.view.focus_position = 0
                    elif self.view.focus_position == 0:
                        self.view.focus_position = 1

            elif current_focus == 'header':
                self.log_frame.focus_position = 'body'

        # View options
        elif self.key_bindings.is_bound_to(input, 'toggle-help'):
            self.toggle_help_panel()
        elif self.key_bindings.is_bound_to(input, 'toggle-toolbar'):
            self.toggle_toolbar()
        elif self.key_bindings.is_bound_to(input, 'toggle-follow'):
            self.toggle_follow()
        elif self.key_bindings.is_bound_to(input, 'toggle-filter'):
            self.toggle_filter_panel()
        elif self.key_bindings.is_bound_to(input, 'clear-filter'):
            self.clear_filters()
        elif self.key_bindings.is_bound_to(input, 'toggle-timestamps'):
            self.toggle_timestamps()
        elif self.key_bindings.is_bound_to(input, 'toggle-levels'):
            self.toggle_levels()
        elif self.key_bindings.is_bound_to(input, 'toggle-wrapping'):
            self.toggle_wrapping()
        elif self.key_bindings.is_bound_to(input, 'toggle-borders'):
            self.toggle_border()
        elif self.key_bindings.is_bound_to(input, 'toggle-sorting'):
            self.toggle_sorting()

        elif self.key_bindings.is_bound_to(input, 'search'):
            self.start_search()
        elif self.key_bindings.is_bound_to(input, 'search-clear'):
            if self.searching:
                self.clear_search_term()

        # Save current file
        elif self.key_bindings.is_bound_to(input, 'save'):
            pass

        # Reload original file
        elif self.key_bindings.is_bound_to(input, 'reload'):
            pass

    def create_header(self, message=""):
        left_header = [
            ('header_success', "[Logs]"),
        ]
        if self.log_listbox:
            left_header.append(
                ('header', " Entries: {}".format(len(self.logs))))
        return urwid.AttrMap(
            urwid.Columns([
                urwid.Text(left_header),
                # urwid.Text([
                #     ('header_warning', " ??? "),
                # ], align="center"),
                urwid.Text(('header_file', "{} {} ".format(
                    message, self.log_file_path.as_posix())),
                           align='right')
            ]),
            'header')

    def create_toolbar(self):
        return urwid.AttrMap(
            urwid.Columns([
                urwid.Padding(urwid.AttrMap(
                    urwid.CheckBox([('header_file', 'F'), 'ollow'],
                                   state=(self.follow_new_log_entries),
                                   on_state_change=self.toggle_follow),
                    'header', 'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.CheckBox([('header_file', 'T'), 'ime'],
                                   state=(self.display_timestamps),
                                   on_state_change=self.toggle_timestamps),
                    'header', 'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.CheckBox([('header_file', 'L'), 'evels'],
                                   state=(self.display_levels),
                                   on_state_change=self.toggle_levels),
                    'header', 'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.CheckBox([('header_file', 'w'), 'rapping'],
                                   state=(self.wrapping[0] == 'space'),
                                   on_state_change=self.toggle_wrapping),
                    'header', 'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.CheckBox([('header_file', 'b'), 'orders'],
                                   state=(self.border[0] == 'bordered'),
                                   on_state_change=self.toggle_border),
                    'header', 'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.Button([('header_file', 'R'), 'eload'],
                                 on_press=self.reload_from_file), 'header',
                    'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.Button([('header_file', 's'), 'ort: ' +
                                  self.sorting_display[self.sorting[0]]],
                                 on_press=self.toggle_sorting), 'header',
                    'plain_selected'),
                              right=2),
                urwid.Padding(urwid.AttrMap(
                    urwid.Button([('header_file', 'f'), 'ilter'],
                                 on_press=self.toggle_filter_panel), 'header',
                    'plain_selected'),
                              right=2)
            ]), 'header')

    def create_footer(self):
        if self.searching:
            self.search_box = SearchWidget(self,
                                           self.key_bindings,
                                           edit_text=self.search_string)
            w = urwid.AttrMap(
                urwid.Columns([
                    (1, urwid.Text('/')), self.search_box,
                    (16,
                     urwid.AttrMap(
                         urwid.Button([('header_file', 'C'), 'lear Search'],
                                      on_press=self.clear_search_term),
                         'header', 'plain_selected'))
                ]), 'footer')
            urwid.connect_signal(self.search_box, 'change',
                                 self.search_box_updated)
        else:
            w = None
        return w

    def create_help_panel(self):
        key_column_width = 12
        header_highlight = 'plain_selected'
        return urwid.AttrMap(
            urwid.LineBox(
                urwid.Padding(
                    urwid.ListBox(
                        # self.key_bindings,
                        [urwid.Divider()] + [
                            urwid.AttrWrap(urwid.Text("""
General
""".strip()), header_highlight)
                        ] +
                        # [ urwid.Divider(u'─') ] +
                        [
                            urwid.Text("""
{0} - show / hide this help message
{1} - quit and save
{2} - show / hide toolbar
{3} - toggle word wrapping
{4} - toggle borders around log lines
{6} - reload the log file
""".format(
                                self.key_bindings["toggle-help"].ljust(
                                    key_column_width),
                                self.key_bindings["quit"].ljust(
                                    key_column_width),
                                self.key_bindings["toggle-toolbar"].ljust(
                                    key_column_width),
                                self.key_bindings["toggle-wrapping"].ljust(
                                    key_column_width),
                                self.key_bindings["toggle-borders"].ljust(
                                    key_column_width),
                                self.key_bindings["save"].ljust(
                                    key_column_width),
                                self.key_bindings["reload"].ljust(
                                    key_column_width),
                            ))
                        ] + [
                            urwid.AttrWrap(
                                urwid.Text("""
Movement
""".strip()), header_highlight)
                        ] +
                        # [ urwid.Divider(u'─') ] +
                        [
                            urwid.Text("""
{0} - select any line, checkbox or button
{1} - move selection down
{2} - move selection up
{3} - move selection to the top item
{4} - move selection to the bottom item
{5} - move selection between logs and filter panel
{6}
{7} - toggle focus between logs, filter panel, and toolbar
""".format(
                                "mouse click".ljust(key_column_width),
                                self.key_bindings["down"].ljust(
                                    key_column_width),
                                self.key_bindings["up"].ljust(
                                    key_column_width),
                                self.key_bindings["top"].ljust(
                                    key_column_width),
                                self.key_bindings["bottom"].ljust(
                                    key_column_width),
                                self.key_bindings["left"].ljust(
                                    key_column_width),
                                self.key_bindings["right"].ljust(
                                    key_column_width),
                                self.key_bindings["change-focus"].ljust(
                                    key_column_width),
                            ))
                        ] + [
                            urwid.AttrWrap(urwid.Text("""
Sorting
""".strip()), header_highlight)
                        ] +
                        # [ urwid.Divider(u'─') ] +
                        [
                            urwid.Text("""
{0} - toggle sort order (Unsorted, Ascending, Descending)
               sort order is saved on quit
""".format(self.key_bindings["toggle-sorting"].ljust(key_column_width), ))
                        ] + [
                            urwid.AttrWrap(
                                urwid.Text("""
Filtering
""".strip()), header_highlight)
                        ] +
                        # [ urwid.Divider(u'─') ] +
                        [
                            urwid.Text("""
{0} - open / close the filtering panel
{1} - clear any active filters
""".format(
                                self.key_bindings["toggle-filter"].ljust(
                                    key_column_width),
                                self.key_bindings["clear-filter"].ljust(
                                    key_column_width),
                            ))
                        ] + [
                            urwid.AttrWrap(
                                urwid.Text("""
Searching
""".strip()), header_highlight)
                        ] +
                        # [ urwid.Divider(u'─') ] +
                        [
                            urwid.Text("""
{0} - start search
{1} - finalize search
{2} - clear search
""".format(
                                self.key_bindings["search"].ljust(
                                    key_column_width),
                                self.key_bindings["search-end"].ljust(
                                    key_column_width),
                                self.key_bindings["search-clear"].ljust(
                                    key_column_width),
                            ))
                        ]),
                    left=1,
                    right=1,
                    min_width=10),
                title='Key Bindings'),
            'default')

    def create_filter_panel(self):
        w = urwid.AttrMap(
            urwid.Padding(
                urwid.ListBox(
                    [
                        urwid.Pile(
                            # self.key_bindings,
                            [
                                urwid.Text('Contexts & Projects',
                                           align='center')
                            ] + [urwid.Divider(u'─')] + [
                                urwid.AttrWrap(
                                    urwid.CheckBox(
                                        c,
                                        state=(c in self.active_contexts),
                                        on_state_change=self.checkbox_clicked,
                                        user_data=['context', c]),
                                    'context_dialog_color', 'context_selected')
                                for c in ["aFilter1", "aFilter2"]
                            ] + [urwid.Divider(u'─')] + [
                                urwid.AttrWrap(
                                    urwid.CheckBox(
                                        p,
                                        state=(p in self.active_projects),
                                        on_state_change=self.checkbox_clicked,
                                        user_data=['project', p]),
                                    'project_dialog_color', 'project_selected')
                                for p in ["bFilter1", "bFilter2"]
                            ] + [urwid.Divider(u'─')] + [
                                urwid.AttrMap(
                                    urwid.Button(['Clear Filters'],
                                                 on_press=self.clear_filters),
                                    'dialog_color', 'plain_selected')
                            ])
                    ] + [urwid.Divider()], ),
                left=1,
                right=1,
                min_width=10),
            'dialog_color')

        bg = urwid.AttrWrap(urwid.SolidFill(u" "),
                            'dialog_background')  # u"\u2592"
        shadow = urwid.AttrWrap(urwid.SolidFill(u" "), 'dialog_shadow')

        bg = urwid.Overlay(shadow, bg, ('fixed left', 2), ('fixed right', 1),
                           ('fixed top', 2), ('fixed bottom', 1))
        w = urwid.Overlay(w, bg, ('fixed left', 1), ('fixed right', 2),
                          ('fixed top', 1), ('fixed bottom', 2))
        return w

    def update_filter_panel(self):
        self.filter_panel = self.create_filter_panel()
        if len(self.view.widget_list) > 1:
            self.view.widget_list.pop()
            self.view.widget_list.append(self.filter_panel)

    def update_header(self, message=""):
        if self.toolbar_is_open:
            self.log_frame.header = urwid.Pile(
                [self.create_header(message),
                 self.create_toolbar()])
        else:
            self.log_frame.header = self.create_header(message)

    def update_footer(self, message=""):
        self.log_frame.footer = self.create_footer()

    def setup_log_data_handler(self, a, b):
        _LOG.debug(_pretty_format(["setup_log_data_handler", a, b]))
        if self.log_file_path.is_file() and self.log_file_proc is None:
            write_fd = self.loop.watch_pipe(self.handle_log_data)
            self.log_file_proc = subprocess.Popen(
                ["tail", "-F", self.log_file_path.as_posix()],
                stdout=write_fd,
                close_fds=True)
        else:
            self.loop.set_alarm_in(1, self.setup_log_data_handler)
            _LOG.debug("Device logfile doesn't exist: '%s'",
                       self.log_file_path.as_posix())

    def handle_log_data(self, data):
        _LOG.debug(_pretty_format(["handle_log_data", data]))
        # TODO(tonymd) detokenize logs here
        log_line = data
        if self.detokenizer:
            log_line = detokenize_base64(self.detokenizer, data)

        for line in log_line.decode(errors="surrogateescape").splitlines():
            self.logs.append(LogLine(line))
            self.append_new_log_widget(self.logs[-1])

        self.update_header()
        if self.follow_new_log_entries:
            self.move_selection_bottom()

    def run(self,
            enable_borders=False,
            enable_word_wrap=False,
            show_toolbar=True,
            show_filter_panel=False):

        urwid.set_encoding('utf8')
        self.log_header = self.create_header()
        self.log_footer = self.create_footer()

        self.log_listbox = LogListBox(urwid.SimpleListWalker([]),
                                      parent_app=self)
        # self.load_logs_from_file()
        self.create_log_widgets()
        self.log_frame = urwid.Frame(urwid.AttrMap(self.log_listbox, 'plain'),
                                     header=self.log_header,
                                     footer=self.log_footer)

        self.terminal_frame_header = urwid.AttrMap(
            urwid.Columns([
                urwid.Text([
                    ('header_success', "[ipython]"),
                ]),
                urwid.Text(('header_file', "{0}".format(os.getcwd())),
                           align='right')
            ]), 'header')

        terminal_command = ["ipython"]
        if self.terminal_args:
            terminal_command = self.terminal_args
        self.ipython_terminal = urwid.Terminal(terminal_command,
                                               encoding='utf-8')

        self.terminal_frame = urwid.Frame(self.ipython_terminal,
                                          header=self.terminal_frame_header)

        self.log_and_terminal_pile = urwid.Pile([
            ('weight', 2, self.log_frame),
            ('weight', 2, self.terminal_frame),
        ])

        self.view = urwid.Columns([
            ('weight', 2, self.log_and_terminal_pile),
        ])

        self.loop = urwid.MainLoop(self.view,
                                   self.palette,
                                   unhandled_input=self.keystroke)
        self.loop.screen.set_terminal_properties(colors=256)

        self.ipython_terminal.main_loop = self.loop

        # self.toggle_wrapping()
        # self.toggle_wrapping()

        if enable_borders:
            self.toggle_border()
        if enable_word_wrap:
            self.toggle_wrapping()
        if show_toolbar:
            self.toggle_toolbar()
        if show_filter_panel:
            self.toggle_filter_panel()

        self.loop.set_alarm_in(1, self.setup_log_data_handler)

        self.loop.run()
        if self.log_file_proc:
            self.log_file_proc.kill()
