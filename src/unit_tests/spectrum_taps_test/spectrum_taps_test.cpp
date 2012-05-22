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

#include "spectrum_taps_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/eq_32_band_graphical.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/utility/rta_core.hpp"
#include "../../domain/utility/spectrum_taps.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <random>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
constexpr uint32_t BlockFrames = 256;

using Tap = SpectrumTaps::Tap;

//! Index of the band covering @p frequency, from the normalised log-frequency spans the core draws at.
size_t bandAt(const RtaCore & core, double frequency)
{
    const auto positions = core.bandLogPositions();
    const double x = std::log10(frequency / 20.0) / std::log10(1000.0);
    for (size_t i = 0; i < positions.size(); i++) {
        if (x >= positions[i].first && x < positions[i].second) {
            return i;
        }
    }
    return 0;
}

//! @p seconds of white noise, or of silence, through @p effect a block at a time.
void feed(Effect & effect, double seconds, bool silent = false)
{
    static std::mt19937 generator { 7 };
    std::normal_distribution<double> noise { 0.0, 0.1 };
    std::vector<double> stereo(BlockFrames * 2);
    const auto blocks = static_cast<size_t>(seconds * SampleRate / BlockFrames);
    for (size_t block = 0; block < blocks; block++) {
        for (uint32_t i = 0; i < BlockFrames; i++) {
            stereo[i * 2] = stereo[i * 2 + 1] = silent ? 0.0 : noise(generator);
        }
        AudioContext context { std::span(stereo.data(), stereo.size()), BlockFrames, static_cast<uint32_t>(SampleRate) };
        effect.process(context);
    }
}

//! Post minus Pre at @p frequency, in dB.
double postMinusPreDb(SpectrumTaps & taps, double frequency)
{
    const auto pre = taps.core(Tap::Pre);
    const auto post = taps.core(Tap::Post);
    const auto band = bandAt(*post, frequency);
    return post->bandMagnitudesDb().at(band) - pre->bandMagnitudesDb().at(band);
}

void setParameter(Effect & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! An EQ 32 boosting the three sliders around 1 kHz by 12 dB, so that the analyzer band at 1 kHz
//! lies on a plateau rather than on the slope of a single third-octave bell.
std::unique_ptr<Eq32BandGraphical> boostedEq32(bool linearPhase)
{
    auto eq = std::make_unique<Eq32BandGraphical>();
    eq->setSampleRate(SampleRate);
    for (const size_t band : { 17, 18, 19 }) { // 800 Hz, 1 kHz, 1.25 kHz
        setParameter(*eq, Constants::NahdXml::xmlKeyBandGain(band), Eq32BandGraphical::sliderParameterValue(12.0));
    }
    setParameter(*eq, Constants::NahdXml::xmlKeyLinearPhase(), linearPhase ? 1.0f : 0.0f);
    return eq;
}

} // namespace

void SpectrumTapsTest::test_core_beforeEnabled_shouldBeNull()
{
    SpectrumTaps taps;
    QVERIFY(!taps.isEnabled());
    QVERIFY(!taps.core(Tap::Pre));
    QVERIFY(!taps.core(Tap::Post));
}

void SpectrumTapsTest::test_setEnabled_preOnly_shouldCreateOnlyPreCore()
{
    SpectrumTaps taps;
    taps.setEnabled(true, false);
    QVERIFY(taps.isEnabled());
    QVERIFY(taps.core(Tap::Pre));
    QVERIFY(!taps.core(Tap::Post));
}

void SpectrumTapsTest::test_setEnabled_off_shouldKeepCoreAndStopRunning()
{
    Eq8BandParametric eq;
    eq.setSampleRate(SampleRate);
    eq.spectrumTaps().setEnabled(false, true);
    feed(eq, 0.5);
    const auto core = eq.spectrumTaps().core(Tap::Post);
    const auto levels = core->bandMagnitudesDb();

    eq.spectrumTaps().setEnabled(false, false);
    feed(eq, 0.5, true);

    QCOMPARE(eq.spectrumTaps().core(Tap::Post), core);
    QCOMPARE(core->bandMagnitudesDb(), levels);
}

void SpectrumTapsTest::test_isSettled_eq8_whileEnabled_shouldBeFalse()
{
    Eq8BandParametric eq;
    QVERIFY(eq.isSettled());
    eq.spectrumTaps().setEnabled(false, true);
    QVERIFY(!eq.isSettled());
    eq.spectrumTaps().setEnabled(false, false);
    QVERIFY(eq.isSettled());
}

void SpectrumTapsTest::test_isSettled_eq32_whileEnabled_shouldBeFalse()
{
    Eq32BandGraphical eq;
    QVERIFY(eq.isSettled());
    eq.spectrumTaps().setEnabled(true, false);
    QVERIFY(!eq.isSettled());
}

void SpectrumTapsTest::test_postMinusPre_eq8_flat_shouldBeZero()
{
    Eq8BandParametric eq;
    eq.setSampleRate(SampleRate);
    eq.spectrumTaps().setEnabled(true, true);
    feed(eq, 2.0);

    for (const double frequency : { 50.0, 200.0, 1000.0, 5000.0, 15000.0 }) {
        const auto difference = postMinusPreDb(eq.spectrumTaps(), frequency);
        QVERIFY2(std::abs(difference) < 0.01, qPrintable(QString::number(frequency) + " Hz: " + QString::number(difference)));
    }
}

void SpectrumTapsTest::test_postMinusPre_eq32_boost_shouldMatchResponse()
{
    const auto eq = boostedEq32(false);
    eq->spectrumTaps().setEnabled(true, true);
    feed(*eq, 3.0);

    const auto difference = postMinusPreDb(eq->spectrumTaps(), 1000.0);
    const auto expected = eq->magnitudeDbAt(1000.0);
    QVERIFY2(std::abs(difference - expected) < 1.0, qPrintable(QString::number(difference) + " vs " + QString::number(expected)));
}

void SpectrumTapsTest::test_postMinusPre_eq32LinearPhase_boost_shouldMatchResponse()
{
    const auto eq = boostedEq32(true);
    eq->spectrumTaps().setEnabled(true, true);
    feed(*eq, 3.0);

    const auto difference = postMinusPreDb(eq->spectrumTaps(), 1000.0);
    const auto expected = eq->magnitudeDbAt(1000.0);
    QVERIFY2(std::abs(difference - expected) < 1.0, qPrintable(QString::number(difference) + " vs " + QString::number(expected)));
}

void SpectrumTapsTest::test_postMinusPre_eq32LinearPhase_afterSignalStops_shouldStayAligned()
{
    // Flat, so that any difference is timing: without the input held back by the latency, Pre has
    // been falling for 300 ms here while Post is still at full level, and reads over 3 dB apart.
    // Held back, a flat linear-phase filter is an exact delay and the two are the same signal
    auto eq = std::make_unique<Eq32BandGraphical>();
    eq->setSampleRate(SampleRate);
    setParameter(*eq, Constants::NahdXml::xmlKeyLinearPhase(), 1.0f);
    eq->spectrumTaps().setEnabled(true, true);
    feed(*eq, 2.0);
    feed(*eq, 0.3, true);

    for (const double frequency : { 200.0, 1000.0, 5000.0 }) {
        const auto difference = postMinusPreDb(eq->spectrumTaps(), frequency);
        QVERIFY2(std::abs(difference) < 0.1, qPrintable(QString::number(frequency) + " Hz: " + QString::number(difference)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SpectrumTapsTest)
