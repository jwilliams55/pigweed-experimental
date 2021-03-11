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
"""ConsoleApp control class."""

import logging
import re
from datetime import datetime

_LOG = logging.getLogger(__name__)


class LogLine():
    def __init__(self, line, log_level="INF", log_time=None):
        self.raw_line = line.strip()
        self.time = log_time if log_time else datetime.now()
        self.level = log_level
        self.search_matches = None

    def formatted_time(self):
        return self.time.isoformat(sep=" ", timespec="milliseconds")

    def highlight(self, show_timestamp=False, show_log_level=False):
        color_list = []
        if show_timestamp:
            color_list.append(("log_time", f"[{self.formatted_time()}] "))
        if show_log_level:
            color_list.append(("log_level", f"{self.level} "))
        color_list.append(self.raw_line)

        # highlight example
        # else:
        #     words_to_be_highlighted = self.contexts + self.projects
        #     if self.due_date:
        #         words_to_be_highlighted.append("due:" + self.due_date)
        #     if self.creation_date:
        #         words_to_be_highlighted.append(self.creation_date)

        #     if words_to_be_highlighted:
        #         color_list = re.split("(" + "|".join([re.escape(w) for w in words_to_be_highlighted]) + ")", self.raw_line)
        #         for index, w in enumerate(color_list):
        #             if w in self.contexts:
        #                 color_list[index] = ('context', w) if show_contexts else ''
        #             elif w in self.projects:
        #                 color_list[index] = ('project', w) if show_projects else ''
        #             elif w == "due:" + self.due_date:
        #                 color_list[index] = ('due_date', w) if show_due_date else ''
        #             elif w == self.creation_date:
        #                 color_list[index] = ('creation_date', w)

        #     if self.priority and self.priority in "ABCDEF":
        #         color_list = ("priority_{0}".format(self.priority.lower()), color_list)
        #     else:
        #         color_list = ("plain", color_list)

        return color_list

    def highlight_search_matches(self):
        color_list = [self.raw_line]
        if self.search_matches:
            color_list = re.split(
                "(" +
                "|".join([re.escape(match)
                          for match in self.search_matches]) + ")",
                self.raw_line)
            for index, w in enumerate(color_list):
                if w in self.search_matches:
                    color_list[index] = ('search_match', w)
        return color_list
