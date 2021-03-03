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
"""LogLineWidget."""

import logging
import urwid

_LOG = logging.getLogger(__name__)


class SearchWidget(urwid.Edit):
    def __init__(self, parent_ui, key_bindings, edit_text=""):
        self.parent_ui = parent_ui
        self.key_bindings = key_bindings
        super(SearchWidget, self).__init__(edit_text=edit_text)

    def keypress(self, size, key):
        if self.key_bindings.is_bound_to(key, 'search-end'):
            self.parent_ui.finalize_search()
        return super(SearchWidget, self).keypress(size, key)
