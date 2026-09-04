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

#ifndef SCHEDULED_EVENT_TEST_HPP
#define SCHEDULED_EVENT_TEST_HPP

#include <QObject>

namespace noteahead {

class ScheduledEventTest : public QObject
{
    Q_OBJECT

private slots:
    void test_renderBlock_withoutEvents_shouldRenderOnePiece();
    void test_renderBlock_scheduledNote_shouldStartOnItsOwnFrame();
    void test_renderBlock_severalEvents_shouldStartEachOnItsOwnFrame();
    void test_renderBlock_eventInThePast_shouldStartAtTheBlockStart();
    void test_renderBlock_eventBeyondTheBlock_shouldWait();
    void test_renderBlock_shouldNotQuantiseToTheBlock_acrossABurst();
    void test_scheduledEvents_shouldBeApplied_inTheOrderQueued();
    void test_clearScheduledEvents_shouldDropEverythingQueued();
};

} // namespace noteahead

#endif // SCHEDULED_EVENT_TEST_HPP
