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

#ifndef UNIT_TESTS_JU_SYNTH_08_TEST_HPP
#define UNIT_TESTS_JU_SYNTH_08_TEST_HPP

#include <QObject>

class JuSynth08Test : public QObject
{
    Q_OBJECT

private slots:
    void test_instantiation();
    void test_polyModeNoteOnAndOff();
    void test_soloModeLegatoGlide();
    void test_unisonModeDetune();
    void test_vcfSlopeSwitching();
    void test_vco2HardSync();
    void test_crossModulationFm();
    void test_presetLoadingDoesNotDeadlock();
    void test_advanceIdle_shouldKeepTheLfoRunning();
    void test_xmlSerialization();
};

#endif // UNIT_TESTS_JU_SYNTH_08_TEST_HPP
