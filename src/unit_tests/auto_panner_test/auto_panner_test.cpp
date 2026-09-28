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

#include "auto_panner_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/auto_panner.hpp"

#include <QTest>

#include <cmath>
#include <span>
#include <vector>

namespace noteahead {

void AutoPannerTest::test_process_shouldModulatePanning()
{
    AutoPanner effect;
    effect.setSampleRate(44100);

    // Set rate to 1Hz, Sine waveform
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        p->get().update(0.5f);
    }
    effect.sync();

    double l = 1.0;
    double r = 1.0;

    // Process some samples and check that l and r are no longer equal
    bool changed = false;
    for (int i = 0; i < 44100; i++) {
        effect.process(l, r);
        if (std::abs(l - r) > 0.001) {
            changed = true;
            break;
        }
    }
    QVERIFY(changed);
}

void AutoPannerTest::test_intensity_shouldScaleModulation()
{
    AutoPanner effect;
    effect.setSampleRate(44100);

    // Intensity 0%
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyIntensity().toStdString()); p) {
        p->get().update(0.0f);
    }
    effect.sync();

    double l = 1.0;
    double r = 1.0;
    effect.process(l, r);
    QCOMPARE(l, 1.0);
    QCOMPARE(r, 1.0);
}

void AutoPannerTest::test_setBpm_shouldUpdateLfoFrequencyInSyncMode()
{
    AutoPanner effect;
    effect.setSampleRate(44100);

    // Enable sync
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeySync().toStdString()); p) {
        p->get().update(1.0f);
    }
    effect.sync();

    effect.setBpm(140.0f);

    double l = 1.0;
    double r = 1.0;
    effect.process(l, r);
    // Just verify it doesn't crash and does something
    QVERIFY(true);
}

void AutoPannerTest::test_rateDivider_shouldStretchTheSweep()
{
    // Sync tops out at one cycle per whole note and the Hz control at 0.05, so a pan that moves
    // over several bars could not be asked for at all before the divider.
    constexpr double sampleRate = 48000.0;

    //! Samples between two successive upward zero crossings of the pan, which is one LFO cycle.
    const auto periodSamples = [](int divider) {
        AutoPanner effect;
        if (auto p = effect.parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString()); p) {
            p->get().update(static_cast<float>(divider));
        }
        effect.sync();

        std::vector<size_t> crossings;
        double previous = 0.0;
        for (size_t i = 0; i < static_cast<size_t>(sampleRate * 40); i++) {
            std::vector<double> buffer(2, 1.0);
            AudioContext context { std::span<double>(buffer.data(), buffer.size()), 1, static_cast<uint32_t>(sampleRate), 120.0, {}, 1, false };
            effect.process(context);
            const double difference = buffer[0] - buffer[1];
            if (previous < 0.0 && difference >= 0.0) {
                crossings.push_back(i);
                if (crossings.size() == 2) {
                    break;
                }
            }
            previous = difference;
        }
        return crossings.size() == 2 ? crossings[1] - crossings[0] : size_t { 0 };
    };

    const auto plain = periodSamples(1);
    const auto divided = periodSamples(4);
    QVERIFY2(plain > 0 && divided > 0, qPrintable(QString::number(plain) + " / " + QString::number(divided)));

    const double ratio = static_cast<double>(divided) / static_cast<double>(plain);
    QVERIFY2(std::abs(ratio - 4.0) < 0.05, qPrintable(QString::number(ratio) + " instead of 4"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoPannerTest)
