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
"""TitleLineBox."""

import logging
import urwid

_LOG = logging.getLogger(__name__)


class TitleLineBox(urwid.WidgetDecoration, urwid.WidgetWrap):
    def __init__(self,
                 original_widget,
                 top_left_title="",
                 bottom_right_title="",
                 border_color='plain',
                 tlcorner=u'┌',
                 tline=u'─',
                 lline=u'│',
                 trcorner=u'┐',
                 blcorner=u'└',
                 rline=u'│',
                 bline=u'─',
                 brcorner=u'┘'):
        """
        Draw a line around original_widget.

        Use 'title' to set an initial title text with will be centered
        on top of the box.

        You can also override the widgets used for the lines/corners:
            tline: top line
            bline: bottom line
            lline: left line
            rline: right line
            tlcorner: top left corner
            trcorner: top right corner
            blcorner: bottom left corner
            brcorner: bottom right corner
        """

        tline, bline = urwid.AttrMap(urwid.Divider(tline),
                                     border_color), urwid.AttrMap(
                                         urwid.Divider(bline), border_color)
        lline, rline = urwid.AttrMap(urwid.SolidFill(lline),
                                     border_color), urwid.AttrMap(
                                         urwid.SolidFill(rline), border_color)
        tlcorner, trcorner = urwid.AttrMap(urwid.Text(tlcorner),
                                           border_color), urwid.AttrMap(
                                               urwid.Text(trcorner),
                                               border_color)
        blcorner, brcorner = urwid.AttrMap(urwid.Text(blcorner),
                                           border_color), urwid.AttrMap(
                                               urwid.Text(brcorner),
                                               border_color)

        self.ttitle_widget = urwid.Text(top_left_title)
        self.tline_widget = urwid.Columns([
            ('fixed', 1, tline),
            ('flow', self.ttitle_widget),
            tline,
        ])
        self.btitle_widget = urwid.Text(bottom_right_title)
        self.bline_widget = urwid.Columns([
            bline,
            ('flow', self.btitle_widget),
            ('fixed', 1, bline),
        ])

        middle = urwid.Columns([
            ('fixed', 1, lline),
            original_widget,
            ('fixed', 1, rline),
        ],
                               box_columns=[0, 2],
                               focus_column=1)

        top = urwid.Columns([('fixed', 1, tlcorner), self.tline_widget,
                             ('fixed', 1, trcorner)])

        bottom = urwid.Columns([('fixed', 1, blcorner), self.bline_widget,
                                ('fixed', 1, brcorner)])

        pile = urwid.Pile([('flow', top), middle, ('flow', bottom)],
                          focus_item=1)

        urwid.WidgetDecoration.__init__(self, original_widget)
        urwid.WidgetWrap.__init__(self, pile)
