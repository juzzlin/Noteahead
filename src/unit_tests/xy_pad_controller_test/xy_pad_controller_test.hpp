// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef XY_PAD_CONTROLLER_TEST_HPP
#define XY_PAD_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class XyPadControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_setDevice_otherDevice_shouldBeRefused();
    void test_panel_write_shouldReachThatPadOnly();
    void test_panel_target_shouldSetAndClear();
    void test_panel_targetShared_withAnLfo_shouldWarn();
    void test_press_shouldReachTheDevice();
    void test_deviceChange_shouldNotifyThePanels();
    void test_renderer_press_shouldPutTheHandThere();
    void test_renderer_dragPastTheEdge_shouldPinThePadToTheEdge();
    void test_renderer_release_shouldLetGo();
};

} // namespace noteahead

#endif // XY_PAD_CONTROLLER_TEST_HPP
