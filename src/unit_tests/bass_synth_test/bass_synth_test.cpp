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

#include "bass_synth_test.hpp"

#include "../../domain/devices/bass_synth_device.hpp"
#include "../../common/constants.hpp"

#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void BassSynthTest::test_serialization()
{
    BassSynthDevice synth("Test BassSynth");
    synth.setLpfCutoff(0.75f);
    synth.setWaveform(PolyBlepOscillator::Waveform::Square);
    synth.setSubLevel(0.5f);

    QString xml;
    QXmlStreamWriter writer(&xml);
    synth.serializeToXml(writer);

    BassSynthDevice synth2("Restored BassSynth");
    QXmlStreamReader reader(xml);
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lpfCutoff(), 0.75f);
    QCOMPARE(static_cast<int>(synth2.waveform()), static_cast<int>(PolyBlepOscillator::Waveform::Square));
    QCOMPARE(synth2.subLevel(), 0.5f);
}

void BassSynthTest::test_midiProcessing()
{
    BassSynthDevice synth("Test BassSynth");
    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    synth.processMidiNoteOff(60);
    // Audio might still be active due to release, but let's test all notes off
    synth.processMidiAllNotesOff();
    QVERIFY(!synth.hasActiveAudio());
}

void BassSynthTest::test_legatoSlide()
{
    BassSynthDevice synth("Test BassSynth");
    synth.setSlide(0.5f);
    
    synth.processMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());
    
    // Trigger another note while first is active
    synth.processMidiNoteOn(72, 100);
    // Monophonic, so still active
    QVERIFY(synth.hasActiveAudio());
}

void BassSynthTest::test_velocityAndAccent()
{
    BassSynthDevice synth("Test BassSynth");
    
    // Normal velocity
    synth.processMidiNoteOn(60, 80);
    // How to verify internal state? Maybe just ensure it doesn't crash
    // Ideally we'd check internal voice accent flag, but it's private.
    
    // Accent velocity
    synth.processMidiNoteOn(60, 110);
    QVERIFY(synth.hasActiveAudio());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassSynthTest)
