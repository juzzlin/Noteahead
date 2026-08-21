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

#include "tip_service_test.hpp"

#include "../../application/service/tip_service.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void TipServiceTest::test_currentTip_notEditing_shouldTellHowToStartEditing()
{
    TipService tipService;

    // The tip that used to be pushed into the status queue at startup and flushed by the first
    // MIDI port message. It is now simply true whenever the editor is not in edit mode.
    QVERIFY(tipService.currentTip().contains("ESC"));
}

void TipServiceTest::test_currentTip_editing_shouldDescribeTheNoteKeys()
{
    TipService tipService;

    tipService.setEditMode(true);

    QVERIFY(!tipService.currentTip().isEmpty());
    QVERIFY(tipService.currentTip() != TipService {}.currentTip());
}

void TipServiceTest::test_setEditMode_changed_shouldEmitSignal()
{
    TipService tipService;
    QSignalSpy spy { &tipService, &TipService::currentTipChanged };

    tipService.setEditMode(true);

    QCOMPARE(spy.count(), 1);
}

void TipServiceTest::test_setEditMode_sameValue_shouldNotEmitSignal()
{
    TipService tipService;
    QSignalSpy spy { &tipService, &TipService::currentTipChanged };

    tipService.setEditMode(false);

    QCOMPARE(spy.count(), 0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TipServiceTest)
