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

#include "sampler_test.hpp"
#include "../../domain/devices/sampler.hpp"
#include "../../common/constants.hpp"

#include <memory>
#include <vector>

using namespace noteahead;

void SamplerTest::testInitialState()
{
    const Sampler sampler;
    QCOMPARE(sampler.name(), Constants::samplerDeviceName().toStdString());
    for (int i = 0; i < 128; i++) {
        QVERIFY(sampler.sample(static_cast<uint8_t>(i)) == nullptr);
    }
}

void SamplerTest::testLoadAndClearSample()
{
    Sampler sampler;
    // We can't easily test loadSample without a real file on disk
    // but we can test that clearSample works (even on null)
    sampler.clearSample(60);
    QVERIFY(sampler.sample(60) == nullptr);
}

void SamplerTest::testMidiNoteOn()
{
    Sampler sampler;
    // Without a loaded sample, note on should do nothing
    sampler.processMidiNoteOn(60, 100);

    std::vector<float> buffer(256, 0.0f);
    sampler.processAudio(buffer.data(), 128, 44100);

    for (const auto val : buffer) {
        QCOMPARE(val, 0.0f);
    }
}

void SamplerTest::testMidiAllNotesOff()
{
    Sampler sampler;
    sampler.processMidiAllNotesOff();
    // Mostly just ensuring it doesn't crash
}

void SamplerTest::testProcessAudio()
{
    Sampler sampler;
    std::vector<float> buffer(256, 1.0f); // Pre-fill with 1.0 to check mixing
    sampler.processAudio(buffer.data(), 128, 44100);

    // Should still be 1.0 since no samples are loaded
    for (const auto val : buffer) {
        QCOMPARE(val, 1.0f);
    }
}

QTEST_MAIN(SamplerTest)
