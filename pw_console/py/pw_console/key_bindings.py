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
"""Console key bindings."""

# from dataclasses import dataclass, field
# from typing import List, Dict


class KeyBindings:
    """Console key bindings."""
    def __init__(self, user_keys={}):
        self.key_bindings = {}
        self.user_keys = user_keys
        self.fillWithDefault()
        self.fillWithUserKeys(user_keys)

    def fillWithUserKeys(self, users_keys):
        for bind in users_keys:
            key = self.userKeysToList(users_keys[bind])
            try:
                self.key_bindings[bind] = key
            except KeyError:
                print("KeyBind \"" + bind + "\" not found")

    def fillWithDefault(self):
        self.key_bindings['toggle-help'] = ['?']
        self.key_bindings['quit'] = ['q']
        self.key_bindings['toggle-toolbar'] = ['t']
        self.key_bindings['toggle-timestamps'] = ['T']
        self.key_bindings['toggle-levels'] = ['L']
        self.key_bindings['toggle-borders'] = ['b']
        self.key_bindings['toggle-wrapping'] = ['w']
        self.key_bindings['save'] = ['S']
        self.key_bindings['reload'] = ['R']
        self.key_bindings['scroll-page-down'] = ['page down']
        self.key_bindings['scroll-page-up'] = ['page up']
        self.key_bindings['down'] = ['j', 'down', 'n']
        self.key_bindings['up'] = ['k', 'up', 'e']
        self.key_bindings['top'] = ['g']
        self.key_bindings['right'] = ['l', 'right']
        self.key_bindings['left'] = ['h', 'left']
        self.key_bindings['bottom'] = ['G']
        self.key_bindings['change-focus'] = ['tab']
        self.key_bindings['toggle-complete'] = ['x']
        self.key_bindings['archive'] = ['X']
        self.key_bindings['insert-after'] = ['o']
        self.key_bindings['insert-before'] = ['O']
        self.key_bindings['priority-up'] = ['p']
        self.key_bindings['priority-down'] = ['P']
        self.key_bindings['save-item'] = ['enter']
        self.key_bindings['edit'] = ['enter']
        self.key_bindings['delete'] = ['D']
        self.key_bindings['swap-down'] = ['J']
        self.key_bindings['swap-up'] = ['K']
        self.key_bindings['edit-complete'] = ['tab']
        self.key_bindings['edit-save'] = ['return']
        self.key_bindings['edit-move-left'] = ['left']
        self.key_bindings['edit-move-right'] = ['right']
        self.key_bindings['edit-word-left'] = ['meta b', 'ctrl b']
        self.key_bindings['edit-word-right'] = ['meta f', 'ctrl f']
        self.key_bindings['edit-end'] = ['ctrl e', 'end']
        self.key_bindings['edit-home'] = ['ctrl a', 'home']
        self.key_bindings['edit-delete-word'] = ['ctrl w']
        self.key_bindings['edit-delete-end'] = ['ctrl k']
        self.key_bindings['edit-delete-beginning'] = ['ctrl u']
        self.key_bindings['edit-paste'] = ['ctrl y']
        self.key_bindings['toggle-filter'] = ['f']
        self.key_bindings['clear-filter'] = ['_']
        self.key_bindings['toggle-follow'] = ['F']
        self.key_bindings['toggle-sorting'] = ['s']
        self.key_bindings['search'] = ['/']
        self.key_bindings['search-end'] = ['enter']
        self.key_bindings['search-clear'] = ['C']

    def __getitem__(self, index):
        return ", ".join(self.key_bindings[index])

    def userKeysToList(self, userKey):
        keys = userKey.split(',')
        return [key.strip() for key in keys]

    def getKeyBinding(self, bind):
        try:
            return self.key_bindings[bind]
        except KeyError:
            return []

    def is_bound_to(self, key, bind):
        return key in self.getKeyBinding(bind)
