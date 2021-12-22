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

#ifndef KICK_909_TEST_HPP
#define KICK_909_TEST_HPP

#include <QObject>

namespace noteahead {

class Kick909Test : public QObject
{
    Q_OBJECT

private slots:
    void test_engine_init_shouldSettleAtTheRd9Pitch();
    void test_engine_sweep_shouldStartAsTheRd9Does();
    void test_engine_tune_shouldLengthenTheSweep();
    void test_engine_decay_shouldMatchTheRd9();
    void test_engine_decay_short_shouldStopDead();
    void test_engine_decay_shouldSwellOverTheFirstCycles();
    void test_engine_attack_low_shouldLeaveTheClick();
    void test_engine_attack_full_shouldRaiseTheClick();
    void test_engine_pitch_shouldMoveTheWholeSweep();
    void test_engine_pitchDepth_zero_shouldFallToZeroHertz();
    void test_engine_pitchDepth_full_shouldRaiseTheEnd();
    void test_engine_drive_full_shouldKeepTheBody();
    void test_engine_trigger_shouldFallSilent();
    void test_presets_every_shouldMatchInitsBody();
    void test_device_chromatic_shouldMovePitchWithTheNote();
    void test_device_ampEnvelope_shouldShortenTheHit();
    void test_preview_render_shouldDrawTheHit();
};

} // namespace noteahead

#endif // KICK_909_TEST_HPP
