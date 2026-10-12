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

#ifndef FM_DRUMS_CONTROLLER_TEST_HPP
#define FM_DRUMS_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class FmDrumsControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_setDevice_otherDrumMachine_shouldBeRefused();
    void test_knob_write_shouldReachTheSelectedPadOnly();
    void test_choice_write_shouldStoreThePlainIndex();
    void test_selectedVoice_change_shouldFollowTheNewPad();
    void test_operator_write_shouldReachTheSelectedPad();
    void test_operator_routing_shouldFollowTheAlgorithm();
    void test_deviceChange_shouldNotifyTheOperators();
};

} // namespace noteahead

#endif // FM_DRUMS_CONTROLLER_TEST_HPP
