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

#ifndef KICK_SYNTH_TEST_HPP
#define KICK_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class KickSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_engine_trigger_shouldSoundAndFallSilent();
    void test_engine_hardHit_shouldStartSharperThanASoftOne();
    void test_engine_damping_shouldShortenTheDrum();
    void test_engine_frontHead_solid_shouldRingLongerThanPorted();
    void test_engine_beater_hard_shouldClickMoreThanFelt();
    void test_engine_retrigger_damped_shouldNotJumpTheTail();
    void test_presets_every_shouldPeakNearInit();
    void test_device_noteOff_withoutEnvelope_shouldLetTheDrumRing();
    void test_device_ampEnvelope_shouldShortenTheHit();
    void test_preview_render_shouldDrawTheHit();
};

} // namespace noteahead

#endif // KICK_SYNTH_TEST_HPP
