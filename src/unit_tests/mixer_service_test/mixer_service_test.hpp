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

#ifndef MIXER_SERVICE_TEST_HPP
#define MIXER_SERVICE_TEST_HPP

#include <QTest>

namespace noteahead {

class MixerServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_muteColumn_shouldSetColumnMuted();
    void test_soloColumn_shouldSetColumnSoloed();

    void test_muteTrack_shouldSetTrackMuted();
    void test_soloTrack_shouldSetTrackSoloed();

    void test_update_shouldUpdatePlaybackState();

    void test_columnShouldNotPlayIfParentTrackMuted();

    void test_clear_shouldSendConfigurationChange();

    void test_setColumnVelocityScale_shouldAffectEffectiveVelocity();
    void test_setTrackVelocityScale_shouldAffectEffectiveVelocity();
    void test_effectiveVelocity_shouldCombineScales();
};

} // namespace noteahead

#endif // MIXER_SERVICE_TEST_HPP
