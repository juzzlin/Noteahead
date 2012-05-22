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

#include "eq_32_band_graphical.hpp"

#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"
#include "eq_32_band_graphical_presets.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace noteahead {

namespace {

//! The highest rate the input tap's alignment delay is allocated for. Above it the Pre spectrum
//! runs early by the difference, which is all that goes wrong.
constexpr double SpectrumMaxSampleRate = 192000.0;

//! The ISO nominal centres the panel is labelled with. The bands run at the exact thirds of an
//! octave GraphicEqDesigner::centreFrequency() gives, which these only round.
constexpr std::array<const char *, Eq32BandGraphical::BandCount> NominalLabels {
    "16 Hz", "20 Hz", "25 Hz", "31.5 Hz", "40 Hz", "50 Hz", "63 Hz", "80 Hz",
    "100 Hz", "125 Hz", "160 Hz", "200 Hz", "250 Hz", "315 Hz", "400 Hz", "500 Hz",
    "630 Hz", "800 Hz", "1 kHz", "1.25 kHz", "1.6 kHz", "2 kHz", "2.5 kHz", "3.15 kHz",
    "4 kHz", "5 kHz", "6.3 kHz", "8 kHz", "10 kHz", "12.5 kHz", "16 kHz", "20 kHz"
};

} // namespace

Eq32BandGraphical::Eq32BandGraphical()
  : m_spectrumTaps { linearPhaseLatencyFrames(SpectrumMaxSampleRate) }
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    for (size_t i = 0; i < BandCount; i++) {
        addParameter(Parameter { C::xmlKeyBandGain(i).toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
        // A band's name is its frequency, which is not translated
        setParameterDisplay(C::xmlKeyBandGain(i).toStdString(), { .unit = "dB", .min = -MaxGainDb, .max = MaxGainDb, .rest = Display::Rest::Centre, .label = nominalLabel(i) });
    }
    addParameter(Parameter { C::xmlKeyStereoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
    // Off, which is also what a project from before the switch existed gets
    addParameter(Parameter { C::xmlKeyLinearPhase().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    setParameterDisplay(C::xmlKeyGain().toStdString(), { .unit = "dB", .min = -MaxGainDb, .max = MaxGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Output Gain") });

    syncParameters();
}

std::string Eq32BandGraphical::nominalLabel(size_t band)
{
    return band < NominalLabels.size() ? NominalLabels.at(band) : std::string {};
}

double Eq32BandGraphical::sliderGainDb(float parameterValue)
{
    return (static_cast<double>(std::clamp(parameterValue, 0.0f, 1.0f)) * 2.0 - 1.0) * MaxGainDb;
}

float Eq32BandGraphical::sliderParameterValue(double gainDb)
{
    return static_cast<float>(std::clamp((gainDb / MaxGainDb + 1.0) * 0.5, 0.0, 1.0));
}

Eq32BandGraphical::Settings Eq32BandGraphical::settingsFromParameters() const
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & key, float fallback) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : fallback;
    };

    Settings settings;
    GraphicEqDesigner::Gains sliders {};
    for (size_t i = 0; i < BandCount; i++) {
        sliders.at(i) = sliderGainDb(valueOf(C::xmlKeyBandGain(i), 0.5f));
    }
    settings.filterGainsDb = GraphicEqDesigner::filterGains(sliders, m_sampleRate);
    settings.stereoMode = static_cast<StereoMode>(std::clamp(static_cast<int>(std::round(valueOf(C::xmlKeyStereoMode(), 0.0f))), 0, 2));
    settings.outputGain = std::pow(10.0, sliderGainDb(valueOf(C::xmlKeyGain(), 0.5f)) / 20.0);
    settings.linearPhase = valueOf(C::xmlKeyLinearPhase(), 0.0f) >= 0.5f;
    return settings;
}

void Eq32BandGraphical::syncParameters()
{
    const auto settings = settingsFromParameters();
    for (auto && bands : m_paths) {
        for (size_t i = 0; i < BandCount; i++) {
            bands.at(i).calculate(GraphicEqDesigner::centreFrequency(i), m_sampleRate, GraphicEqDesigner::bandQ(), settings.filterGainsDb.at(i));
        }
    }
    m_stereoMode = settings.stereoMode;
    m_outputGain = settings.outputGain;

    if (settings.linearPhase) {
        if (!m_linearPhase) {
            // Engaged: nothing of the minimum-phase path's state carries over
            m_firIsIdle = true;
            m_quietFrames = 0;
        }
        startLinearPhaseDesign();
    }
    m_linearPhase = settings.linearPhase;
}

void Eq32BandGraphical::startLinearPhaseDesign()
{
    const auto rate = m_sampleRate;
    if (rate <= 0.0) {
        return;
    }
    m_fir.configure(rate);
    if (m_bypassDelay.size() != m_fir.latencyFrames()) {
        m_bypassDelay.assign(m_fir.latencyFrames(), 0.0);
        m_bypassIndex = 0;
    }
    std::vector<MatchedBellFilter::SquaredMagnitude> sections;
    sections.reserve(BandCount);
    for (auto && band : m_paths.at(0)) {
        if (!band.isBypassed()) {
            sections.push_back(band.squaredMagnitude());
        }
    }
    if (m_firIsIdle) {
        m_fir.reset();
        std::ranges::fill(m_bypassDelay, 0.0);
    }
    // Built a slice at a time even from cold, which takes a few thousand frames: well inside the
    // latency, during which nothing but the kernel's faint pre-ringing reaches the output anyway,
    // and the convolution keeps no state of its own output, so the finished kernel takes over as
    // though it had been there from the start
    m_fir.startDesign(sections, 1.0);
}

void Eq32BandGraphical::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers) {
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldUpdateBuffers = false;
        m_shouldSyncParameters = true;
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
}

double Eq32BandGraphical::processPath(Bands & bands, double input)
{
    double output = input;
    for (auto && band : bands) {
        output = band.process(output);
    }
    return output;
}

void Eq32BandGraphical::processFrame(double & left, double & right)
{
    switch (m_stereoMode) {
    case StereoMode::Stereo:
        // Left and right through identical bands, which is the same as mid and side through them
        // and leaves a flat panel bit for bit transparent: the sum and difference would not.
        left = processPath(m_paths.at(0), left);
        right = processPath(m_paths.at(1), right);
        break;
    case StereoMode::Mid:
    case StereoMode::Side: {
        double mid = (left + right) * 0.5;
        double side = (left - right) * 0.5;
        if (m_stereoMode == StereoMode::Mid) {
            mid = processPath(m_paths.at(0), mid);
        } else {
            side = processPath(m_paths.at(1), side);
        }
        left = mid + side;
        right = mid - side;
        break;
    }
    }
    left *= m_outputGain;
    right *= m_outputGain;
}

void Eq32BandGraphical::processFrameLinearPhase(double & left, double & right)
{
    m_quietFrames = (left != 0.0 || right != 0.0) ? 0 : m_quietFrames + 1;
    switch (m_stereoMode) {
    case StereoMode::Stereo:
        m_fir.process(left, right);
        break;
    case StereoMode::Mid:
    case StereoMode::Side: {
        double mid = (left + right) * 0.5;
        double side = (left - right) * 0.5;
        double & shaped = m_stereoMode == StereoMode::Mid ? mid : side;
        double & passed = m_stereoMode == StereoMode::Mid ? side : mid;
        // The filter's second channel has nothing to do in these modes
        double unused = 0.0;
        m_fir.process(shaped, unused);
        const double delayed = m_bypassDelay[m_bypassIndex];
        m_bypassDelay[m_bypassIndex] = passed;
        m_bypassIndex = (m_bypassIndex + 1) % m_bypassDelay.size();
        passed = delayed;
        left = mid + side;
        right = mid - side;
        break;
    }
    }
    left *= m_outputGain;
    right *= m_outputGain;
}

void Eq32BandGraphical::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }
    updateBuffers();
    if (m_linearPhase) {
        m_fir.advanceDesign(1);
        processFrameLinearPhase(left, right);
        m_firIsIdle = false;
    } else {
        processFrame(left, right);
    }
}

void Eq32BandGraphical::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }
    updateBuffers();
    if (m_linearPhase) {
        m_spectrumTaps.processPre(context, m_fir.latencyFrames());
        m_fir.advanceDesign(context.frameCount);
        for (uint32_t i = 0; i < context.frameCount; i++) {
            processFrameLinearPhase(context.buffer[i * 2], context.buffer[i * 2 + 1]);
        }
        m_firIsIdle = false;
        m_spectrumTaps.processPost(context);
        return;
    }
    m_spectrumTaps.processPre(context);
    for (uint32_t i = 0; i < context.frameCount; i++) {
        processFrame(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
    m_spectrumTaps.processPost(context);
}

void Eq32BandGraphical::reset()
{
    for (auto && bands : m_paths) {
        for (auto && band : bands) {
            band.reset();
        }
    }
    m_fir.reset();
    std::ranges::fill(m_bypassDelay, 0.0);
    m_firIsIdle = true;
    m_quietFrames = 0;
}

bool Eq32BandGraphical::isSettled() const
{
    if (m_spectrumTaps.isEnabled()) {
        return false;
    }
    // The whole kernel, and the partition being gathered, has to have run out of signal
    return !m_linearPhase || m_firIsIdle || m_quietFrames > m_fir.kernelLength() + 2 * m_fir.partitionSize();
}

size_t Eq32BandGraphical::latencyFrames(double sampleRate) const
{
    // Off the parameter rather than m_linearPhase, which is brought up to date only when a block
    // is processed: a render asks before it has processed anything
    const auto p = parameter(Constants::NahdXml::xmlKeyLinearPhase().toStdString());
    return p && p->get().value() >= 0.5f ? linearPhaseLatencyFrames(sampleRate) : 0;
}

size_t Eq32BandGraphical::linearPhaseLatencyFrames(double sampleRate)
{
    return LinearPhaseFir::latencyFramesFor(sampleRate);
}

SpectrumTaps & Eq32BandGraphical::spectrumTaps()
{
    return m_spectrumTaps;
}

void Eq32BandGraphical::sync()
{
    m_shouldUpdateBuffers = true;
}

std::vector<double> Eq32BandGraphical::magnitudesDbAt(std::span<const double> frequencies) const
{
    // Designed on bands of its own: the processing ones belong to the audio thread, and are only
    // brought up to date when a block is processed, so a dialog opened before anything played would
    // otherwise draw a flat line.
    const auto settings = settingsFromParameters();
    std::array<MatchedBellFilter, BandCount> bands;
    for (size_t i = 0; i < BandCount; i++) {
        bands.at(i).calculate(GraphicEqDesigner::centreFrequency(i), m_sampleRate, GraphicEqDesigner::bandQ(), settings.filterGainsDb.at(i));
    }
    const double outputDb = 20.0 * std::log10(settings.outputGain);

    std::vector<double> result;
    result.reserve(frequencies.size());
    for (const double frequency : frequencies) {
        double db = outputDb;
        for (auto && band : bands) {
            db += band.magnitudeDbAt(frequency, m_sampleRate);
        }
        result.push_back(db);
    }
    return result;
}

double Eq32BandGraphical::magnitudeDbAt(double frequency) const
{
    return magnitudesDbAt(std::span { &frequency, 1 }).front();
}

const EffectPresetList & Eq32BandGraphical::factoryPresets() const
{
    return Eq32BandGraphicalPresets::presets();
}

std::string Eq32BandGraphical::typeIdString()
{
    return "e5a0c7d2-3f19-4b8e-a641-9c2d7b30f8e1";
}

std::string Eq32BandGraphical::type() const
{
    return Constants::RackEffectType::eq32BandGraphical().toStdString();
}

std::string Eq32BandGraphical::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
