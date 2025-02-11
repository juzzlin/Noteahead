// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "mixer_service_test.hpp"

#include "../../application/mixer_service.hpp"

#include <QSignalSpy>

namespace noteahead {

void MixerServiceTest::test_muteColumn_shouldSetColumnMuted()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy columnMutedSpy(&mixerService, &MixerService::columnMuted);

    mixerService.muteColumn(0, 1, true);
    QCOMPARE(mixerService.isColumnMuted(0, 1), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), false);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(columnMutedSpy.count(), 1);

    mixerService.muteColumn(0, 1, false);
    QCOMPARE(mixerService.isColumnMuted(0, 1), false);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(columnMutedSpy.count(), 2);
}

void MixerServiceTest::test_soloColumn_shouldSetColumnSoloed()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy columnSoloedSpy(&mixerService, &MixerService::columnSoloed);

    mixerService.soloColumn(0, 1, true);
    QCOMPARE(mixerService.isColumnSoloed(0, 1), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(columnSoloedSpy.count(), 1);

    mixerService.soloColumn(0, 1, false);
    QCOMPARE(mixerService.isColumnSoloed(0, 1), false);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(columnSoloedSpy.count(), 2);
}

void MixerServiceTest::test_muteTrack_shouldSetTrackMuted()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy trackMutedSpy(&mixerService, &MixerService::trackMuted);

    mixerService.muteTrack(0, true);
    QCOMPARE(mixerService.isTrackMuted(0), true);
    QCOMPARE(mixerService.shouldTrackPlay(0), false);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), false);
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(trackMutedSpy.count(), 1);

    mixerService.muteTrack(0, false);
    QCOMPARE(mixerService.isTrackMuted(0), false);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(trackMutedSpy.count(), 2);
}

void MixerServiceTest::test_soloTrack_shouldSetTrackSoloed()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy trackSoloedSpy(&mixerService, &MixerService::trackSoloed);

    mixerService.soloTrack(0, true);
    QCOMPARE(mixerService.isTrackSoloed(0), true);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(trackSoloedSpy.count(), 1);

    mixerService.soloTrack(0, false);
    QCOMPARE(mixerService.isTrackSoloed(0), false);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(trackSoloedSpy.count(), 2);
}

void MixerServiceTest::test_update_shouldUpdatePlaybackState()
{
    MixerService mixerService;
    mixerService.soloColumn(0, 1, true);
    mixerService.update();
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
}

void MixerServiceTest::test_columnShouldNotPlayIfParentTrackMuted()
{
    MixerService mixerService;
    mixerService.muteTrack(0, true);
    QCOMPARE(mixerService.shouldTrackPlay(0), false);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), false);

    mixerService.muteTrack(0, false);
    QCOMPARE(mixerService.shouldTrackPlay(0), true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 1), true);
}

void MixerServiceTest::test_clear_shouldSendConfigurationChange()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);

    mixerService.muteTrack(1, true);
    mixerService.soloTrack(2, true);
    mixerService.muteColumn(3, 0, true);
    mixerService.soloColumn(4, 1, true);

    mixerService.clear();

    QCOMPARE(configurationChangedSpy.count(), 4); // Initial changes + clear
    QVERIFY(!mixerService.isTrackMuted(1));
    QVERIFY(!mixerService.isTrackSoloed(2));
    QVERIFY(!mixerService.isColumnMuted(3, 0));
    QVERIFY(!mixerService.isColumnSoloed(4, 1));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MixerServiceTest)
