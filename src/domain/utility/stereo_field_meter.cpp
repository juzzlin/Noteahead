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

#include "stereo_field_meter.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Where the three measured bands are divided. Fixed rather than adjustable: these are the regions
//! width behaves differently in, not a crossover anything is summed back across.
constexpr double lowerCrossoverHz = 250.0;
constexpr double upperCrossoverHz = 3000.0;

//! What the Speed control selects, in milliseconds. Fast follows a phrase, Slow follows a mix.
constexpr std::array<double, 3> meterTimeConstantsMs { 80.0, 300.0, 1000.0 };

//! Anything quieter than this is silence as far as the readings are concerned.
constexpr double silenceFloor = 1.0e-9;

//! Floor the level readings rest on, so a silent meter does not report minus infinity.
constexpr float floorDb = -100.0f;

} // namespace

StereoFieldMeter::StereoFieldMeter()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeySpeed().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyZoom().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyShowGuides().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });

    syncParameters();
}

void StereoFieldMeter::PairMeter::update(double a, double b, double coefficient)
{
    productAb += (a * b - productAb) * coefficient;
    squareA += (a * a - squareA) * coefficient;
    squareB += (b * b - squareB) * coefficient;
}

float StereoFieldMeter::PairMeter::correlation() const
{
    const double energy = std::sqrt(squareA * squareB);

    // Silence has no correlation to report. Reading it as centred is what keeps the meter still
    // between notes instead of wandering on the noise floor.
    if (energy < silenceFloor) {
        return 1.0f;
    }

    return static_cast<float>(std::clamp(productAb / energy, -1.0, 1.0));
}

double StereoFieldMeter::PairMeter::rmsA() const
{
    return std::sqrt(std::max(squareA, 0.0));
}

double StereoFieldMeter::PairMeter::rmsB() const
{
    return std::sqrt(std::max(squareB, 0.0));
}

void StereoFieldMeter::PairMeter::reset()
{
    productAb = 0.0;
    squareA = 0.0;
    squareB = 0.0;
}

void StereoFieldMeter::Splitter::setCutoffs(double lowerFrequency, double upperFrequency, double sampleRate)
{
    lowSplit.setSampleRate(sampleRate);
    highSplit.setSampleRate(sampleRate);
    lowSplit.setCutoff(lowerFrequency);
    highSplit.setCutoff(upperFrequency);
}

void StereoFieldMeter::Splitter::split(double input, std::array<double, NumBands> & bands)
{
    double low = 0.0;
    double aboveLow = 0.0;
    lowSplit.process(input, low, aboveLow);

    double mid = 0.0;
    double high = 0.0;
    highSplit.process(aboveLow, mid, high);

    bands[0] = low;
    bands[1] = mid;
    bands[2] = high;
}

void StereoFieldMeter::Splitter::reset()
{
    lowSplit.reset();
    highSplit.reset();
}

std::string StereoFieldMeter::typeIdString()
{
    return "7e1c94b6-2fa8-4d53-9c6b-05e8d3417a92";
}

std::string StereoFieldMeter::type() const
{
    return Constants::RackEffectType::stereoFieldMeter().toStdString();
}

std::string StereoFieldMeter::typeId() const
{
    return typeIdString();
}

void StereoFieldMeter::setAnalysisEnabled(bool enabled)
{
    m_analysisEnabled.store(enabled, std::memory_order_relaxed);
    m_scope.setActive(enabled);
}

void StereoFieldMeter::analyse(double left, double right)
{
    m_broadband.update(left, right, m_meterCoefficient);

    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;
    m_midSide.update(mid, side, m_meterCoefficient);

    std::array<double, NumBands> bandsLeft {};
    std::array<double, NumBands> bandsRight {};
    m_splitterL.split(left, bandsLeft);
    m_splitterR.split(right, bandsRight);

    for (size_t i = 0; i < NumBands; i++) {
        m_bands[i].update(bandsLeft[i], bandsRight[i], m_meterCoefficient);
    }
}

void StereoFieldMeter::processSample(double & left, double & right)
{
    if (!m_analysisEnabled.load(std::memory_order_relaxed) || m_sampleRate <= 0) {
        return;
    }

    updateState();
    analyse(left, right);
}

void StereoFieldMeter::processBlock(AudioContext & context)
{
    if (!m_analysisEnabled.load(std::memory_order_relaxed)) {
        return;
    }

    if (context.sampleRate != m_lastSampleRate) {
        m_lastSampleRate = context.sampleRate;
        m_shouldSyncParameters = true;
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        analyse(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }

    m_scope.write(context.buffer.data(), context.frameCount, context.sampleRate);

    // Published once per block rather than per sample: the dialog reads it thirty times a second,
    // and taking the lock for every frame would be the only expensive thing this effect does.
    Reading reading;
    reading.correlation = m_broadband.correlation();
    for (size_t i = 0; i < NumBands; i++) {
        reading.bandCorrelation[i] = m_bands[i].correlation();
    }
    reading.midDb = std::max(Utils::Dsp::linearToDb(static_cast<float>(m_midSide.rmsA())), floorDb);
    reading.sideDb = std::max(Utils::Dsp::linearToDb(static_cast<float>(m_midSide.rmsB())), floorDb);

    const double leftRms = m_broadband.rmsA();
    const double rightRms = m_broadband.rmsB();
    const double total = leftRms + rightRms;
    reading.balance = total > silenceFloor ? static_cast<float>((rightRms - leftRms) / total) : 0.0f;

    {
        const std::lock_guard<std::mutex> lock { m_readingMutex };
        m_reading = reading;
    }
}

StereoFieldMeter::Reading StereoFieldMeter::reading() const
{
    const std::lock_guard<std::mutex> lock { m_readingMutex };
    return m_reading;
}

AudioScope::Snapshot StereoFieldMeter::trace(size_t maxPoints) const
{
    return m_scope.snapshot(maxPoints);
}

void StereoFieldMeter::updateState()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldSyncParameters) {
        syncParameters();
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldSyncParameters = false;
    }
}

void StereoFieldMeter::syncParameters()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : (m_lastSampleRate > 0 ? m_lastSampleRate : 48000.0);

    m_splitterL.setCutoffs(lowerCrossoverHz, upperCrossoverHz, sampleRate);
    m_splitterR.setCutoffs(lowerCrossoverHz, upperCrossoverHz, sampleRate);

    size_t speedIndex = 1;
    if (const auto p = parameter(Constants::NahdXml::xmlKeySpeed().toStdString()); p) {
        speedIndex = std::min(static_cast<size_t>(std::lround(p->get().value())), meterTimeConstantsMs.size() - 1);
    }

    m_meterCoefficient = 1.0 - std::exp(-1.0 / (meterTimeConstantsMs[speedIndex] * 0.001 * sampleRate));
}

void StereoFieldMeter::reset()
{
    m_broadband.reset();
    m_midSide.reset();
    for (auto && band : m_bands) {
        band.reset();
    }
    m_splitterL.reset();
    m_splitterR.reset();

    const std::lock_guard<std::mutex> lock { m_readingMutex };
    m_reading = Reading {};
}

void StereoFieldMeter::sync()
{
    m_shouldSyncParameters = true;
}

} // namespace noteahead
