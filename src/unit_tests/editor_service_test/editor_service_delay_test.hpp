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

#ifndef EDITOR_SERVICE_DELAY_TEST_HPP
#define EDITOR_SERVICE_DELAY_TEST_HPP

#include <QObject>

namespace noteahead {

class EditorServiceDelayTest : public QObject
{
    Q_OBJECT

private slots:
    void test_requestDigitSetAtCurrentPosition_shouldSetDelay_whenAtDelayColumn();
    void test_cursorNavigation_shouldIncludeDelayColumns();
};

} // namespace noteahead

#endif // EDITOR_SERVICE_DELAY_TEST_HPP
