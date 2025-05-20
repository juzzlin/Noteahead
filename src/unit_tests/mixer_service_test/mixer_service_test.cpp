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

#include "../../application/service/mixer_service.hpp"

#include <QSignalSpy>

namespace noteahead {

void MixerServiceTest::test_muteColumn_shouldSetColumnMuted()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy columnMutedSpy(&mixerService, &MixerService::columnMuted);

    mixerService.muteColumn(0, 1, true);
    QVERIFY(mixerService.isColumnMuted(0, 1));
    QVERIFY(!mixerService.shouldColumnPlay(0, 1));
    QVERIFY(mixerService.shouldTrackPlay(0));
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(columnMutedSpy.count(), 1);

    mixerService.muteColumn(0, 1, false);
    QVERIFY(!mixerService.isColumnMuted(0, 1));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QVERIFY(mixerService.shouldTrackPlay(0));
    QCOMPARE(columnMutedSpy.count(), 2);
}

void MixerServiceTest::test_invertMutedColumns_shouldInvertMutedColumns()
{
    MixerService mixerService;

    connect(&mixerService, &MixerService::trackIndicesRequested, this, [&] {
        mixerService.setTrackIndices({ 0 });
    });

    connect(&mixerService, &MixerService::columnCountOfTrackRequested, this, [&](auto trackIndex) {
        QCOMPARE(trackIndex, 0);
        mixerService.setColumnCount(0, 3);
    });

    mixerService.muteColumn(0, 0, true);
    mixerService.muteColumn(0, 1, true);
    mixerService.muteColumn(0, 2, true);

    mixerService.invertMutedColumns(0, 1);

    QVERIFY(mixerService.isColumnMuted(0, 0));
    QVERIFY(!mixerService.isColumnMuted(0, 1));
    QVERIFY(mixerService.isColumnMuted(0, 2));

    mixerService.invertMutedColumns(0, 1);

    QVERIFY(!mixerService.isColumnMuted(0, 0));
    QVERIFY(!mixerService.isColumnMuted(0, 1));
    QVERIFY(!mixerService.isColumnMuted(0, 2));
}

void MixerServiceTest::test_soloColumn_shouldSetColumnSoloed()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy columnSoloedSpy(&mixerService, &MixerService::columnSoloed);

    mixerService.soloColumn(0, 1, true);
    QVERIFY(mixerService.isColumnSoloed(0, 1));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(columnSoloedSpy.count(), 1);

    mixerService.soloColumn(0, 1, false);
    QVERIFY(!mixerService.isColumnSoloed(0, 1));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(columnSoloedSpy.count(), 2);
}

void MixerServiceTest::test_invertSoloedColumns_shouldInvertSoloedColumns()
{
    MixerService mixerService;

    connect(&mixerService, &MixerService::trackIndicesRequested, this, [&] {
        mixerService.setTrackIndices({ 0 });
    });

    connect(&mixerService, &MixerService::columnCountOfTrackRequested, this, [&](auto trackIndex) {
        QCOMPARE(trackIndex, 0);
        mixerService.setColumnCount(0, 3);
    });

    mixerService.soloColumn(0, 0, true);
    mixerService.soloColumn(0, 1, true);
    mixerService.soloColumn(0, 2, true);

    mixerService.invertSoloedColumns(0, 1);

    QVERIFY(mixerService.isColumnSoloed(0, 0));
    QVERIFY(!mixerService.isColumnSoloed(0, 1));
    QVERIFY(mixerService.isColumnSoloed(0, 2));

    mixerService.invertSoloedColumns(0, 1);

    QVERIFY(!mixerService.isColumnSoloed(0, 0));
    QVERIFY(!mixerService.isColumnSoloed(0, 1));
    QVERIFY(!mixerService.isColumnSoloed(0, 2));
}

void MixerServiceTest::test_muteTrack_shouldSetTrackMuted()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy trackMutedSpy(&mixerService, &MixerService::trackMuted);

    mixerService.muteTrack(0, true);
    QVERIFY(mixerService.isTrackMuted(0));
    QVERIFY(!mixerService.shouldTrackPlay(0));
    QVERIFY(!mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(trackMutedSpy.count(), 1);

    mixerService.muteTrack(0, false);
    QVERIFY(!mixerService.isTrackMuted(0));
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(trackMutedSpy.count(), 2);
}

void MixerServiceTest::test_invertMutedTracks_shouldInvertMutedTracks()
{
    MixerService mixerService;

    connect(&mixerService, &MixerService::trackIndicesRequested, this, [&] {
        mixerService.setTrackIndices({ 0, 1, 2 });
    });

    mixerService.muteTrack(0, true);
    mixerService.muteTrack(1, true);
    mixerService.muteTrack(2, true);

    mixerService.invertMutedTracks(1);

    QVERIFY(mixerService.isTrackMuted(0));
    QVERIFY(!mixerService.isTrackMuted(1));
    QVERIFY(mixerService.isTrackMuted(2));

    mixerService.invertMutedTracks(1);

    QVERIFY(!mixerService.isTrackMuted(0));
    QVERIFY(!mixerService.isTrackMuted(1));
    QVERIFY(!mixerService.isTrackMuted(2));
}

void MixerServiceTest::test_soloTrack_shouldSetTrackSoloed()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);
    QSignalSpy trackSoloedSpy(&mixerService, &MixerService::trackSoloed);

    mixerService.soloTrack(0, true);
    QVERIFY(mixerService.isTrackSoloed(0));
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(configurationChangedSpy.count(), 1);
    QCOMPARE(trackSoloedSpy.count(), 1);

    mixerService.soloTrack(0, false);
    QVERIFY(!mixerService.isTrackSoloed(0));
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QCOMPARE(trackSoloedSpy.count(), 2);
}

void MixerServiceTest::test_invertSoloedTracks_shouldInvertSoloedTracks()
{
    MixerService mixerService;

    connect(&mixerService, &MixerService::trackIndicesRequested, this, [&] {
        mixerService.setTrackIndices({ 0, 1, 2 });
    });

    mixerService.soloTrack(0, true);
    mixerService.soloTrack(1, true);
    mixerService.soloTrack(2, true);

    mixerService.invertSoloedTracks(1);

    QVERIFY(mixerService.isTrackSoloed(0));
    QVERIFY(!mixerService.isTrackSoloed(1));
    QVERIFY(mixerService.isTrackSoloed(2));

    mixerService.invertSoloedTracks(1);

    QVERIFY(!mixerService.isTrackSoloed(0));
    QVERIFY(!mixerService.isTrackSoloed(1));
    QVERIFY(!mixerService.isTrackSoloed(2));
}

void MixerServiceTest::test_update_shouldUpdatePlaybackState()
{
    MixerService mixerService;
    mixerService.soloColumn(0, 1, true);
    mixerService.update();
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
    QVERIFY(mixerService.shouldTrackPlay(0));
}

void MixerServiceTest::test_shouldColumnPlay_muted_shouldNotPlay()
{
    MixerService mixerService;
    mixerService.muteColumn(0, 0, true);
    QCOMPARE(mixerService.shouldColumnPlay(0, 0), false);
}

void MixerServiceTest::test_shouldColumnPlay_parentMuted_shouldNotPlay()
{
    MixerService mixerService;
    mixerService.muteTrack(0, true);
    QVERIFY(!mixerService.shouldTrackPlay(0));
    QVERIFY(!mixerService.shouldColumnPlay(0, 1));

    mixerService.muteTrack(0, false);
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 1));
}

void MixerServiceTest::test_shouldColumnPlay_siblingSoloed_shouldNotPlay()
{
    MixerService mixerService;
    mixerService.soloColumn(0, 0, true);

    QVERIFY(mixerService.shouldColumnPlay(0, 0)); // Soloed column should play
    QVERIFY(!mixerService.shouldColumnPlay(0, 1)); // Other columns in the same track should not play
}

void MixerServiceTest::test_shouldColumnPlay_oneOfSoloedColumns_shouldPlay()
{
    MixerService mixerService;
    mixerService.soloColumn(0, 0, true);
    mixerService.soloColumn(0, 1, true);

    QVERIFY(mixerService.shouldColumnPlay(0, 0)); // Soloed column should play
    QVERIFY(mixerService.shouldColumnPlay(0, 1)); // Another soloed column should also play
    QVERIFY(!mixerService.shouldColumnPlay(0, 2)); // Non-soloed columns should not play
}

void MixerServiceTest::test_shouldTrackPlay_muted_shouldNotPlay()
{
    MixerService mixerService;
    mixerService.muteTrack(0, true);
    QVERIFY(!mixerService.shouldTrackPlay(0));
    QVERIFY(!mixerService.shouldColumnPlay(0, 1));
}

void MixerServiceTest::test_shouldTrackPlay_siblingSoloed_shouldNotPlay()
{
    MixerService mixerService;
    mixerService.soloTrack(1, true);
    QVERIFY(!mixerService.shouldTrackPlay(0));
    QVERIFY(!mixerService.shouldColumnPlay(0, 0));
    QVERIFY(mixerService.shouldTrackPlay(1));
    QVERIFY(mixerService.shouldColumnPlay(1, 0));
}

void MixerServiceTest::test_shouldTrackPlay_oneOfSoloedTracks_shouldPlay()
{
    MixerService mixerService;
    mixerService.soloTrack(0, true);
    mixerService.soloTrack(1, true);
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 0));
    QVERIFY(mixerService.shouldTrackPlay(1));
    QVERIFY(mixerService.shouldColumnPlay(1, 0));
    QVERIFY(!mixerService.shouldTrackPlay(2));
}

void MixerServiceTest::test_shouldTrackPlay_oneOfSoloedTracks_siblingHasSoloedColumn_shouldPlay()
{
    MixerService mixerService;
    mixerService.soloTrack(0, true);
    mixerService.soloColumn(0, 0, true);
    mixerService.soloTrack(1, true);
    QVERIFY(mixerService.shouldTrackPlay(0));
    QVERIFY(mixerService.shouldColumnPlay(0, 0));
    QVERIFY(mixerService.shouldTrackPlay(1));
    QVERIFY(mixerService.shouldColumnPlay(1, 0));
    QVERIFY(!mixerService.shouldTrackPlay(2));
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

void MixerServiceTest::test_setColumnVelocityScale_shouldAffectEffectiveVelocity()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);

    mixerService.setColumnVelocityScale(0, 1, 50);
    QCOMPARE(mixerService.columnVelocityScale(0, 1), 50);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 50);
    QCOMPARE(configurationChangedSpy.count(), 1);

    mixerService.setColumnVelocityScale(0, 1, 75);
    QCOMPARE(mixerService.columnVelocityScale(0, 1), 75);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 75);
    QCOMPARE(configurationChangedSpy.count(), 2);
}

void MixerServiceTest::test_setTrackVelocityScale_shouldAffectEffectiveVelocity()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);

    mixerService.setTrackVelocityScale(0, 50);
    QCOMPARE(mixerService.trackVelocityScale(0), 50);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 50);
    QCOMPARE(configurationChangedSpy.count(), 1);

    mixerService.setTrackVelocityScale(0, 25);
    QCOMPARE(mixerService.trackVelocityScale(0), 25);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 25);
    QCOMPARE(configurationChangedSpy.count(), 2);
}

void MixerServiceTest::test_effectiveVelocity_shouldCombineScales()
{
    MixerService mixerService;
    QSignalSpy configurationChangedSpy(&mixerService, &MixerService::configurationChanged);

    mixerService.setTrackVelocityScale(0, 80);
    mixerService.setColumnVelocityScale(0, 1, 50);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 40); // (80 * 50 * 100) / (100 * 100) = 40
    QCOMPARE(configurationChangedSpy.count(), 2);

    mixerService.setTrackVelocityScale(0, 100);
    mixerService.setColumnVelocityScale(0, 1, 100);
    QCOMPARE(mixerService.effectiveVelocity(0, 1, 100), 100);
    QCOMPARE(configurationChangedSpy.count(), 4);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MixerServiceTest)
