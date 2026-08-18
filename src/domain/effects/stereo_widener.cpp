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

#include "stereo_widener.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double minFrequency = 20.0;
constexpr double maxFrequency = 20000.0;

//! Range the Mono Bass corner sweeps. Far enough up to cover the region a club system or a record
//! cutter wants centred, and no further: above this the control would be a tone control rather than
//! a safety net.
constexpr double minMonoHz = 20.0;
constexpr double maxMonoHz = 300.0;

//! Widest a band can be made. Two doubles the side signal, which is as far as this can be pushed
//! before what comes back from a mono fold-down is less than what went in.
constexpr double maxWidth = 2.0;

//! Smallest ratio between the two corners. Crossing them over would invert the band order and leave
//! the middle band with nothing to pass, so the upper one is pushed along instead.
constexpr double minCrossoverSpacing = 1.1;

//! How long the correlation meters average over. Long enough that the reading can be followed by
//! eye rather than flickering with every transient.
constexpr double meterTimeConstantMs = 300.0;

//! Output trim range either side of unity, in dB.
constexpr double outputRangeDb = 12.0;

} // namespace

StereoWidener::StereoWidener()
{
    const auto addContinuous = [this](const QString & key, float internalDefault, int xmlMin, int xmlMax, int xmlScale) {
        addParameter(Parameter { key.toStdString(), internalDefault, xmlMin, xmlMax, Parameter::internalToXmlValue(internalDefault, xmlMin, xmlMax), xmlScale });
    };

    addContinuous(Constants::NahdXml::xmlKeyCrossoverFreq(0), static_cast<float>(ParameterMapper::unmapLogFrequency(250.0, minFrequency, maxFrequency)), 20, 20000, 1);
    addContinuous(Constants::NahdXml::xmlKeyCrossoverFreq(1), static_cast<float>(ParameterMapper::unmapLogFrequency(3000.0, minFrequency, maxFrequency)), 20, 20000, 1);

    for (size_t i = 0; i < NumBands; i++) {
        // Half travel is 100%, so the effect does nothing at all until a band is actually moved.
        addContinuous(Constants::NahdXml::xmlKeyBandWidth(i), 0.5f, 0, 200, 1);
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandSolo(i).toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    }

    addParameter(Parameter { Constants::NahdXml::xmlKeyMonoBass().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addContinuous(Constants::NahdXml::xmlKeyMonoFreq(), static_cast<float>(ParameterMapper::unmapLogFrequency(120.0, minMonoHz, maxMonoHz)), 20, 300, 1);

    addContinuous(Constants::NahdXml::xmlKeyGain(), 0.5f, -1200, 1200, 100); // 0 dB

    syncParameters();
}

void StereoWidener::Splitter::setCutoffs(double lowerFrequency, double upperFrequency, double sampleRate)
{
    lowSplit.setSampleRate(sampleRate);
    highSplit.setSampleRate(sampleRate);
    lowAllPass.setSampleRate(sampleRate);

    lowSplit.setCutoff(lowerFrequency);
    highSplit.setCutoff(upperFrequency);
    lowAllPass.setCutoff(upperFrequency);
}

void StereoWidener::Splitter::split(double input, BandFrame & bands)
{
    double low = 0.0;
    double aboveLow = 0.0;
    lowSplit.process(input, low, aboveLow);

    double mid = 0.0;
    double high = 0.0;
    highSplit.process(aboveLow, mid, high);

    // The low band never met the upper crossover, so it is given that crossover's phase rotation and
    // nothing else. Without this the three bands would notch each other around the upper corner.
    bands[0] = lowAllPass.processAllPass(low);
    bands[1] = mid;
    bands[2] = high;
}

void StereoWidener::Splitter::reset()
{
    lowSplit.reset();
    highSplit.reset();
    lowAllPass.reset();
}

void StereoWidener::CorrelationMeter::update(double left, double right, double coefficient)
{
    productLr += (left * right - productLr) * coefficient;
    squareL += (left * left - squareL) * coefficient;
    squareR += (right * right - squareR) * coefficient;
}

float StereoWidener::CorrelationMeter::correlation() const
{
    const double energy = std::sqrt(squareL * squareR);

    // Silence has no correlation to report. Reading it as centred rather than as anything else is
    // what keeps the meter still between notes instead of wandering on the noise floor.
    if (energy < 1.0e-12) {
        return 1.0f;
    }

    return static_cast<float>(std::clamp(productLr / energy, -1.0, 1.0));
}

void StereoWidener::CorrelationMeter::reset()
{
    productLr = 0.0;
    squareL = 0.0;
    squareR = 0.0;
}

void StereoWidener::applyWidth(double & left, double & right, double width)
{
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5 * width;
    left = mid + side;
    right = mid - side;
}

void StereoWidener::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateState();

    BandFrame bandsLeft {};
    BandFrame bandsRight {};

    // Both channels go through their own splitter, but the two are always kept at the same corners:
    // the mid/side arithmetic below only means anything if what it is given has been filtered
    // identically on each side.
    m_splitterL.split(left, bandsLeft);
    m_splitterR.split(right, bandsRight);

    double outLeft = 0.0;
    double outRight = 0.0;

    for (size_t i = 0; i < NumBands; i++) {
        double bandLeft = bandsLeft[i];
        double bandRight = bandsRight[i];

        applyWidth(bandLeft, bandRight, m_bandWidths[i]);

        // Metered after the width control, so that what the meter shows is what the control did.
        m_meters[i].update(bandLeft, bandRight, m_meterCoefficient);

        if (!m_anyBandSoloed || m_bandSoloed[i]) {
            outLeft += bandLeft;
            outRight += bandRight;
        }
    }

    if (m_monoBass) {
        double lowLeft = 0.0;
        double highLeft = 0.0;
        double lowRight = 0.0;
        double highRight = 0.0;
        m_monoSplitL.process(outLeft, lowLeft, highLeft);
        m_monoSplitR.process(outRight, lowRight, highRight);

        // Only the low half is centred, and the high half is put back untouched. Both halves left
        // the same crossover, so they sum flat and the corner itself is inaudible.
        const double lowMono = (lowLeft + lowRight) * 0.5;
        outLeft = lowMono + highLeft;
        outRight = lowMono + highRight;
    }

    left = outLeft * m_outputGain;
    right = outRight * m_outputGain;
}

void StereoWidener::updateState()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldSyncParameters) {
        syncParameters();
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldSyncParameters = false;
    }
}

void StereoWidener::syncParameters()
{
    for (size_t i = 0; i < NumCrossovers; i++) {
        if (const auto p = parameter(Constants::NahdXml::xmlKeyCrossoverFreq(i).toStdString()); p) {
            m_crossoverFrequencies[i] = ParameterMapper::mapLogFrequency(static_cast<double>(p->get().value()), minFrequency, maxFrequency);
        }
    }
    m_crossoverFrequencies[1] = std::max(m_crossoverFrequencies[1], m_crossoverFrequencies[0] * minCrossoverSpacing);

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_splitterL.setCutoffs(m_crossoverFrequencies[0], m_crossoverFrequencies[1], sampleRate);
    m_splitterR.setCutoffs(m_crossoverFrequencies[0], m_crossoverFrequencies[1], sampleRate);

    m_anyBandSoloed = false;

    for (size_t i = 0; i < NumBands; i++) {
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandWidth(i).toStdString()); p) {
            m_bandWidths[i] = static_cast<double>(p->get().value()) * maxWidth;
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandSolo(i).toStdString()); p) {
            m_bandSoloed[i] = p->get().value() > 0.5f;
            m_anyBandSoloed = m_anyBandSoloed || m_bandSoloed[i];
        }
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyMonoBass().toStdString()); p) {
        m_monoBass = p->get().value() > 0.5f;
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyMonoFreq().toStdString()); p) {
        m_monoFrequency = ParameterMapper::mapLogFrequency(static_cast<double>(p->get().value()), minMonoHz, maxMonoHz);
    }

    m_monoSplitL.setSampleRate(sampleRate);
    m_monoSplitR.setSampleRate(sampleRate);
    m_monoSplitL.setCutoff(m_monoFrequency);
    m_monoSplitR.setCutoff(m_monoFrequency);

    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputGain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>((p->get().value() - 0.5f) * 2.0f * outputRangeDb)));
    }

    m_meterCoefficient = 1.0 - std::exp(-1.0 / (meterTimeConstantMs * 0.001 * sampleRate));
}

float StereoWidener::bandCorrelation(size_t bandIndex) const
{
    if (bandIndex >= NumBands) {
        return 1.0f;
    }

    return m_meters[bandIndex].correlation();
}

void StereoWidener::reset()
{
    m_splitterL.reset();
    m_splitterR.reset();
    m_monoSplitL.reset();
    m_monoSplitR.reset();

    for (auto && meter : m_meters) {
        meter.reset();
    }
}

void StereoWidener::sync()
{
    m_shouldSyncParameters = true;
}

std::string StereoWidener::typeIdString()
{
    return "d83d6a9a-8c0f-4344-8dca-9b7e43830d2e";
}

std::string StereoWidener::type() const
{
    return Constants::RackEffectType::stereoWidener().toStdString();
}

std::string StereoWidener::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
