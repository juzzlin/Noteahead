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

#include "multiband_compressor.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>

namespace noteahead {

namespace {

constexpr double minFrequency = 20.0;
constexpr double maxFrequency = 20000.0;

constexpr double minAttackMs = 0.1;
constexpr double maxAttackMs = 500.0;
constexpr double minReleaseMs = 1.0;
constexpr double maxReleaseMs = 2000.0;

//! Smallest ratio between the two corners. Crossing them over would invert the band order and leave
//! the middle band with nothing to pass, so the upper one is pushed along instead.
constexpr double minCrossoverSpacing = 1.1;

} // namespace

MultibandCompressor::MultibandCompressor()
{
    const auto addContinuous = [this](const QString & key, float internalDefault, int xmlMin, int xmlMax, int xmlScale) {
        addParameter(Parameter { key.toStdString(), internalDefault, xmlMin, xmlMax, Parameter::internalToXmlValue(internalDefault, xmlMin, xmlMax), xmlScale });
    };

    addContinuous(Constants::NahdXml::xmlKeyCrossoverFreq(0), static_cast<float>(ParameterMapper::unmapLogFrequency(200.0, minFrequency, maxFrequency)), 20, 20000, 1);
    addContinuous(Constants::NahdXml::xmlKeyCrossoverFreq(1), static_cast<float>(ParameterMapper::unmapLogFrequency(2000.0, minFrequency, maxFrequency)), 20, 20000, 1);

    for (size_t i = 0; i < NumBands; i++) {
        addContinuous(Constants::NahdXml::xmlKeyBandThreshold(i), 2.0f / 3.0f, -6000, 0, 100); // -20 dB
        addContinuous(Constants::NahdXml::xmlKeyBandRatio(i), 3.0f / 19.0f, 100, 2000, 100); // 4:1
        addContinuous(Constants::NahdXml::xmlKeyBandKnee(i), 0.25f, 0, 2400, 100); // 6 dB
        addContinuous(Constants::NahdXml::xmlKeyBandAttack(i), static_cast<float>(ParameterMapper::unmapExponential(10.0, minAttackMs, maxAttackMs)), 0, 1000, 1000);
        addContinuous(Constants::NahdXml::xmlKeyBandRelease(i), static_cast<float>(ParameterMapper::unmapExponential(100.0, minReleaseMs, maxReleaseMs)), 0, 1000, 1000);
        addContinuous(Constants::NahdXml::xmlKeyBandMakeup(i), 0.5f, -1200, 1200, 100); // 0 dB
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandBypass(i).toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
        addParameter(Parameter { Constants::NahdXml::xmlKeyBandSolo(i).toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    }

    addContinuous(Constants::NahdXml::xmlKeyGain(), 0.5f, -1200, 1200, 100); // 0 dB
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });

    syncParameters();
}

void MultibandCompressor::Splitter::setCutoffs(double lowerFrequency, double upperFrequency, double sampleRate)
{
    lowSplit.setSampleRate(sampleRate);
    highSplit.setSampleRate(sampleRate);
    lowAllPass.setSampleRate(sampleRate);

    lowSplit.setCutoff(lowerFrequency);
    highSplit.setCutoff(upperFrequency);
    lowAllPass.setCutoff(upperFrequency);
}

void MultibandCompressor::Splitter::split(double input, BandFrame & bands)
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

void MultibandCompressor::Splitter::reset()
{
    lowSplit.reset();
    highSplit.reset();
    lowAllPass.reset();
}

std::optional<size_t> MultibandCompressor::sidechainSourceDeviceIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const int val = p->get().xmlValue(); val >= 0) {
            return static_cast<size_t>(val);
        }
    }
    return std::nullopt;
}

void MultibandCompressor::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateState();
    processFrame(left, right, false, 0.0, 0.0);
}

void MultibandCompressor::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateState();

    const bool hasSidechain = m_sidechainSourceDevice && *m_sidechainSourceDevice < context.deviceOutputBuffers.size();
    const auto sidechainBuffer = hasSidechain ? context.deviceOutputBuffers[*m_sidechainSourceDevice] : std::span<const double> {};
    const bool useSidechain = hasSidechain && !sidechainBuffer.empty();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processFrame(context.buffer[i * 2], context.buffer[i * 2 + 1],
                     useSidechain,
                     useSidechain ? sidechainBuffer[i * 2] : 0.0,
                     useSidechain ? sidechainBuffer[i * 2 + 1] : 0.0);
    }
}

void MultibandCompressor::processFrame(double & left, double & right, bool hasSidechain, double sidechainLeft, double sidechainRight)
{
    BandFrame bandsLeft {};
    BandFrame bandsRight {};
    m_splitterL.split(left, bandsLeft);
    m_splitterR.split(right, bandsRight);

    // Each band's detector hears the matching band of whatever it is listening to, so an external
    // side chain has to be split by the same corners as the signal being compressed.
    BandFrame detectorLeft = bandsLeft;
    BandFrame detectorRight = bandsRight;
    if (hasSidechain) {
        m_sidechainSplitterL.split(sidechainLeft, detectorLeft);
        m_sidechainSplitterR.split(sidechainRight, detectorRight);
    }

    double outLeft = 0.0;
    double outRight = 0.0;

    for (size_t i = 0; i < NumBands; i++) {
        auto && band = m_bands[i];
        double bandLeft = bandsLeft[i];
        double bandRight = bandsRight[i];

        // The detector runs even for a bypassed band, so that toggling bypass picks up the envelope
        // where it already is instead of stepping the gain.
        const double gainDb = band.core.processGainDb(detectorLeft[i], detectorRight[i]);

        if (!band.bypassed) {
            const double gain = Utils::Dsp::dbToLinear(static_cast<float>(gainDb + band.makeupDb));
            bandLeft *= gain;
            bandRight *= gain;
        }

        if (!m_anyBandSoloed || band.soloed) {
            outLeft += bandLeft;
            outRight += bandRight;
        }
    }

    left = outLeft * m_outputGain;
    right = outRight * m_outputGain;
}

void MultibandCompressor::updateState()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldSyncParameters) {
        syncParameters();
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldSyncParameters = false;
    }
}

void MultibandCompressor::syncParameters()
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
    m_sidechainSplitterL.setCutoffs(m_crossoverFrequencies[0], m_crossoverFrequencies[1], sampleRate);
    m_sidechainSplitterR.setCutoffs(m_crossoverFrequencies[0], m_crossoverFrequencies[1], sampleRate);

    auto detectorMode = CompressorCore::DetectorMode::Peak;
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        detectorMode = p->get().value() > 0.5f ? CompressorCore::DetectorMode::Rms : CompressorCore::DetectorMode::Peak;
    }

    m_anyBandSoloed = false;

    for (size_t i = 0; i < NumBands; i++) {
        auto && band = m_bands[i];
        band.core.setSampleRate(sampleRate);
        band.core.setDetectorMode(detectorMode);

        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandThreshold(i).toStdString()); p) {
            band.core.setThresholdDb(-60.0 + static_cast<double>(p->get().value()) * 60.0);
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandRatio(i).toStdString()); p) {
            band.core.setRatio(1.0 + static_cast<double>(p->get().value()) * 19.0);
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandKnee(i).toStdString()); p) {
            band.core.setKneeDb(static_cast<double>(p->get().value()) * 24.0);
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandAttack(i).toStdString()); p) {
            band.core.setAttackMs(ParameterMapper::mapExponential(p->get().value(), minAttackMs, maxAttackMs));
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandRelease(i).toStdString()); p) {
            band.core.setReleaseMs(ParameterMapper::mapExponential(p->get().value(), minReleaseMs, maxReleaseMs));
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandMakeup(i).toStdString()); p) {
            band.makeupDb = -12.0 + static_cast<double>(p->get().value()) * 24.0;
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandBypass(i).toStdString()); p) {
            band.bypassed = p->get().value() > 0.5f;
        }
        if (const auto p = parameter(Constants::NahdXml::xmlKeyBandSolo(i).toStdString()); p) {
            band.soloed = p->get().value() > 0.5f;
            m_anyBandSoloed = m_anyBandSoloed || band.soloed;
        }
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_outputGain = Utils::Dsp::dbToLinear(-12.0f + p->get().value() * 24.0f);
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const int val = p->get().xmlValue(); val >= 0) {
            m_sidechainSourceDevice = static_cast<size_t>(val);
        } else {
            m_sidechainSourceDevice = std::nullopt;
        }
    }
}

float MultibandCompressor::bandReductionDb(size_t bandIndex) const
{
    if (bandIndex >= NumBands) {
        return 0.0f;
    }

    return static_cast<float>(m_bands[bandIndex].core.reductionDb());
}

void MultibandCompressor::reset()
{
    for (auto && band : m_bands) {
        band.core.reset();
    }

    m_splitterL.reset();
    m_splitterR.reset();
    m_sidechainSplitterL.reset();
    m_sidechainSplitterR.reset();
}

void MultibandCompressor::sync()
{
    m_shouldSyncParameters = true;
}

std::string MultibandCompressor::typeIdString()
{
    return "9c4d5e6f-7a8b-4c9d-8e0f-1a2b3c4d5e6f";
}

std::string MultibandCompressor::type() const
{
    return Constants::RackEffectType::multibandCompressor().toStdString();
}

std::string MultibandCompressor::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
