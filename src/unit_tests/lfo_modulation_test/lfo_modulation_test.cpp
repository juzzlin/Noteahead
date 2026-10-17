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

#include "lfo_modulation_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;

constexpr uint32_t SampleRate { 48000 };

const std::string & cutoffKey()
{
    static const auto key = NahdXml::xmlKeyLpfCutoff().toStdString();
    return key;
}

struct Rig
{
    AudioEngine engine;
    std::shared_ptr<LfoDevice> lfo { std::make_shared<LfoDevice>("LFO") };
    std::shared_ptr<SynthDevice> synth { std::make_shared<SynthDevice>("Synth") };

    Rig()
    {
        synth->parameter(cutoffKey())->get().setValue(0.5f);
        lfo->updateLfoParameter(0, NahdXml::xmlKeyWaveform(), static_cast<float>(Lfo::Waveform::Sine));
        lfo->updateLfoParameter(0, NahdXml::xmlKeyRate(), 0.6f);
        lfo->updateLfoParameter(0, NahdXml::xmlKeyIntensity(), 1.0f);
        lfo->setTarget(0, { ParameterAutomation::Target::Owner::Device, synth->uuid(), cutoffKey() });
        engine.setDevice(0, lfo);
        engine.setDevice(1, synth);
    }

    float cutoff() const
    {
        return synth->parameter(cutoffKey())->get().value();
    }

    //! Runs @p frames through the engine in blocks of @p block, calling @p after each block
    void run(uint32_t frames, uint32_t block, const std::function<void()> & after = {})
    {
        std::vector<double> buffer(block * 2);
        for (uint32_t done = 0; done < frames; done += block) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            AudioContext context { std::span(buffer.data(), buffer.size()), block, SampleRate };
            engine.process(context);
            if (after) {
                after();
            }
        }
    }
};

} // namespace

void LfoModulationTest::test_engine_lfoOnSynthCutoff_shouldModulateIt()
{
    Rig rig;
    float lowest = 1.0f;
    float highest = 0.0f;
    rig.run(SampleRate, 256, [&] {
        lowest = std::min(lowest, rig.cutoff());
        highest = std::max(highest, rig.cutoff());
    });

    // Full depth swings half the knob's travel either way of where it is set
    QVERIFY2(lowest < 0.1f && highest > 0.9f, qPrintable(QString { "%1 .. %2" }.arg(lowest).arg(highest)));
    // And the knob stays where the user put it
    QCOMPARE(rig.synth->parameter(cutoffKey())->get().shownValue(), 0.5f);
    QVERIFY(rig.synth->parameter(cutoffKey())->get().isModulated());
}

void LfoModulationTest::test_engine_anyBufferSize_shouldModulateTheSame()
{
    // A render's blocks are not playback's: wherever both have a block boundary, the value agrees
    const auto valuesWith = [](uint32_t block) {
        Rig rig;
        std::vector<float> values;
        uint32_t done = 0;
        rig.run(2048 * 24, block, [&] {
            done += block;
            if (done % 2048 == 0) {
                values.push_back(rig.cutoff());
            }
        });
        return values;
    };
    const auto small = valuesWith(128);
    const auto large = valuesWith(2048);

    QCOMPARE(small.size(), large.size());
    QVERIFY(small.size() > 20);
    for (size_t i = 0; i < small.size(); i++) {
        QCOMPARE(small.at(i), large.at(i));
    }
}

void LfoModulationTest::test_engine_effectTarget_shouldFollow()
{
    Rig rig;
    const auto gain = std::make_shared<Gain>();
    rig.engine.insertEffectRack().setEffect(0, gain);
    const auto gainKey = NahdXml::xmlKeyGain().toStdString();
    rig.lfo->setTarget(1, { ParameterAutomation::Target::Owner::Effect, gain->id(), gainKey });
    rig.lfo->updateLfoParameter(1, NahdXml::xmlKeyIntensity(), 1.0f);

    float lowest = 1.0f;
    float highest = 0.0f;
    rig.run(SampleRate, 256, [&] {
        lowest = std::min(lowest, gain->parameter(gainKey)->get().value());
        highest = std::max(highest, gain->parameter(gainKey)->get().value());
    });

    QVERIFY2(highest - lowest > 0.5f, qPrintable(QString { "%1 .. %2" }.arg(lowest).arg(highest)));
}

void LfoModulationTest::test_engine_removingTheLfo_shouldReleaseItsTarget()
{
    Rig rig;
    rig.run(4800, 256);
    QVERIFY(rig.synth->parameter(cutoffKey())->get().isModulated());

    rig.engine.clearDevice(0);

    QVERIFY(!rig.synth->parameter(cutoffKey())->get().isModulated());
    QCOMPARE(rig.cutoff(), 0.5f);
}

void LfoModulationTest::test_engine_targetChange_shouldReleaseTheOldTarget()
{
    Rig rig;
    rig.run(4800, 256);
    const auto resonanceKey = NahdXml::xmlKeyLpfResonance().toStdString();

    rig.lfo->setTarget(0, { ParameterAutomation::Target::Owner::Device, rig.synth->uuid(), resonanceKey });
    rig.run(4800, 256);

    QVERIFY(!rig.synth->parameter(cutoffKey())->get().isModulated());
    QCOMPARE(rig.cutoff(), 0.5f);
    QVERIFY(rig.synth->parameter(resonanceKey)->get().isModulated());
}

void LfoModulationTest::test_engine_stop_shouldKeepModulating()
{
    // A stop clears automation; a free LFO keeps going so it can be heard while playing live
    Rig rig;
    rig.run(4800, 256);

    rig.engine.clearParameterAutomation();
    const float before = rig.cutoff();
    rig.run(9600, 256);

    QVERIFY(rig.synth->parameter(cutoffKey())->get().isModulated());
    QVERIFY(rig.cutoff() != before);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LfoModulationTest)
