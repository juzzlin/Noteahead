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

#include "stereo_exciter.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../dsp/upsampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

//! Range the Tune control sweeps. The X32 labels this one 1 - 10; real hertz is what every other
//! frequency control in this application shows, so that is what it sweeps here. The span covers
//! everything from lifting a dull upper midrange to adding air over an already bright source.
constexpr double MinTuneHz = 700.0;
constexpr double MaxTuneHz = 8000.0;

//! Q of the side chain's steep path, from a gentle corner to a pronounced emphasis at Tune. Peak is
//! what decides how much of the band right at the corner is fed to the shaper.
constexpr double MinPeakQ = 0.6;
constexpr double MaxPeakQ = 5.0;

//! Q of the gentle path Zero Fill mixes in. Below the corner the steep path leaves a hole; this one
//! fills it back in, so the generated harmonics come from a wider band and read as fuller.
constexpr double FillQ = 0.5;

//! Q of the high pass the generated signal goes through. Butterworth: flat above Tune, no bump.
constexpr double OutputQ = 0.7071;

//! How much of the gentle path Zero Fill can add.
constexpr double MaxFill = 1.0;

//! The band is held at unit peak on its way into the shaper, by following its own envelope, so
//! Harmonics adds the same proportion of it whether the source peaks at -30 dBFS or at full scale. A
//! fixed curve could only suit one level: calibrated for full scale it was a straight line on a
//! device's real -26 dBFS and added nothing.
//!
//! This is how far below the input the band can fall and still be treated as a band. Below that it
//! is measured against the input instead, so that the little a tone far under Tune leaves in the
//! band is not lifted to full strength, which would leave Tune meaning nothing.
constexpr double LowestBandRatio = 0.1;

//! Envelope release. The attack is instant, so the band never outruns what it is measured against,
//! and the release is slow so the measurement does not ride the waveform and modulate it.
constexpr double EnvelopeReleaseMs = 150.0;

//! Room left above the band's envelope for the peaks that fall between samples, which the
//! oversampled path reconstructs and the envelope, taken at the base rate, never sees.
constexpr double PeakHeadroom = 1.2;

//! Lowest envelope the band is measured against. Silence would otherwise be lifted without limit.
constexpr double EnvelopeFloor = 1.0e-4;

//! How much of the generated signal is added at the top of the Harmonics control.
constexpr double MaxHarmonics = 0.8;

//! Response of the meter, which follows what is being generated rather than the audio.
constexpr double MeterReleaseMs = 120.0;

} // namespace

struct StereoExciter::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;
};

StereoExciter::StereoExciter()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyTune().toStdString(), 0.5f, 700, 8000, 2500, 1, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPeak().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyZeroFill().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTimbre().toStdString(), 0.5f, -5000, 5000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHarmonics().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });

    addMixParameter(1.0f);

    // What an exciter adds is easy to overdo precisely because it is hard to hear on its own.
    addSoloParameter();

    syncParameters();
}

StereoExciter::~StereoExciter() = default;

std::string StereoExciter::typeIdString()
{
    return "c95e83b1-4d27-4a60-91f8-6b3a2e7d40c6";
}

std::string StereoExciter::type() const
{
    return Constants::RackEffectType::stereoExciter().toStdString();
}

std::string StereoExciter::typeId() const
{
    return typeIdString();
}

void StereoExciter::updateFilters()
{
    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    if (!m_coefficientsDirty && std::abs(sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }
    m_lastSampleRate = sampleRate;
    m_coefficientsDirty = false;

    const double tuneHz = ParameterMapper::mapLogFrequency(static_cast<double>(m_tune), MinTuneHz, MaxTuneHz);
    const double peakQ = MinPeakQ + (MaxPeakQ - MinPeakQ) * static_cast<double>(m_peak);

    m_steepL.calculateHighCut(tuneHz, sampleRate, peakQ);
    m_steepR.calculateHighCut(tuneHz, sampleRate, peakQ);
    m_gentleL.calculateHighCut(tuneHz * 0.5, sampleRate, FillQ);
    m_gentleR.calculateHighCut(tuneHz * 0.5, sampleRate, FillQ);
    m_outputL.calculateHighCut(tuneHz, sampleRate, OutputQ);
    m_outputR.calculateHighCut(tuneHz, sampleRate, OutputQ);
}

double StereoExciter::sideChain(SvfFilter & steep, SvfFilter & gentle, double input) const
{
    // High pass by subtraction, so Peak shapes the corner of the band that is fed to the shaper
    // without a second filter design. Zero Fill adds back a shallower path, which fills the hole
    // the steep one leaves just below the corner.
    const double steepBand = input - steep.process(input);
    const double gentleBand = input - gentle.process(input);
    return steepBand + gentleBand * static_cast<double>(m_zeroFill) * MaxFill;
}

double StereoExciter::shape(double value, double amplitude) const
{
    // Chebyshev polynomials, generalised to a band of peak @p amplitude: on a sine they return
    // exactly one harmonic and nothing at all at the band's own frequency. A saturating curve also
    // compresses the band it is given, and taking that back out is what an exciter cannot do
    // cleanly: the remainder is a copy of the band, larger than the harmonics themselves.
    const double squaredAmplitude = amplitude * amplitude;

    // The 3rd harmonic, which reads as edge.
    const double odd = 4.0 * value * value * value - 3.0 * squaredAmplitude * value;

    // The 2nd harmonic, which reads as warmth.
    const double even = 2.0 * value * value - squaredAmplitude;

    // Timbre runs odd at one end and even at the other, blending rather than switching.
    const double blend = static_cast<double>(m_timbre);
    return odd * (1.0 - blend) + even * blend;
}

void StereoExciter::processSample(double & left, double & right)
{
    updateFilters();

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;

    // The filters and the envelopes carry state, so they keep running even when nothing is added.
    const double sideL = sideChain(m_steepL, m_gentleL, left);
    const double sideR = sideChain(m_steepR, m_gentleR, right);
    const double releaseCoefficient = std::exp(-1.0 / (EnvelopeReleaseMs * sampleRate / 1000.0));
    const auto follow = [releaseCoefficient](double & envelope, double peak) {
        envelope = std::max(peak, envelope * releaseCoefficient);
    };
    follow(m_inputEnvelope, std::max(std::abs(left), std::abs(right)));
    follow(m_bandEnvelope, std::max(std::abs(sideL), std::abs(sideR)));

    if (m_harmonics <= 0.0f) {
        m_harmonicsDb = 0.0;
        return;
    }

    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const double amount = static_cast<double>(m_harmonics) * MaxHarmonics;

    const double reference = std::max({ m_bandEnvelope * PeakHeadroom, m_inputEnvelope * LowestBandRatio, EnvelopeFloor });
    const double amplitude = std::min(1.0, m_bandEnvelope / reference);
    const auto harmonicsOf = [this, amplitude](double normalised) {
        // Only a peak that beats even the headroom gets here, but the polynomials grow fast past unity.
        return shape(std::clamp(normalised, -1.0, 1.0), amplitude);
    };

    double harmonicL = 0.0;
    double harmonicR = 0.0;

    if (factor == 1) {
        harmonicL = harmonicsOf(sideL / reference);
        harmonicR = harmonicsOf(sideR / reference);
    } else {
        // Harmonics of a band this high land above Nyquist at the base rate and fold back down as
        // inharmonic tones, which is the opposite of what the effect is for.
        //
        // The input itself passes straight through, not delayed to match: Mix and Solo are applied
        // by the base class against the input as it came in, and a delayed copy would comb against
        // that. Only the harmonics are late, and they are new material with nothing to cancel.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(sideL / reference), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(sideR / reference), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            highL[k] = static_cast<float>(harmonicsOf(static_cast<double>(highL[k])));
            highR[k] = static_cast<float>(harmonicsOf(static_cast<double>(highR[k])));
        }
        harmonicL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        harmonicR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    // On a band of many partials the shaper also returns their differences, which land below the band
    // as mud under the source, and the 2nd harmonic leaves an offset whenever the envelope moves.
    // Neither is top end, so only what is above Tune is kept. Then back to the band's own level.
    harmonicL = (harmonicL - m_outputL.process(harmonicL)) * reference;
    harmonicR = (harmonicR - m_outputR.process(harmonicR)) * reference;

    left += harmonicL * amount;
    right += harmonicR * amount;

    const double generated = std::max(std::abs(harmonicL), std::abs(harmonicR)) * amount;
    const double generatedDb = generated > 1.0e-7 ? Utils::Dsp::linearToDb(static_cast<float>(generated)) : -120.0;
    const double meterReleaseCoefficient = std::exp(-1.0 / (MeterReleaseMs * sampleRate / 1000.0));
    if (generatedDb > m_harmonicsDb) {
        m_harmonicsDb = generatedDb;
    } else {
        m_harmonicsDb = meterReleaseCoefficient * m_harmonicsDb + (1.0 - meterReleaseCoefficient) * generatedDb;
    }
}

float StereoExciter::harmonicsDb() const
{
    return static_cast<float>(m_harmonicsDb);
}

void StereoExciter::reset()
{
    m_steepL.reset();
    m_steepR.reset();
    m_gentleL.reset();
    m_gentleR.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
    m_outputL.reset();
    m_outputR.reset();
    m_harmonicsDb = 0.0;
    m_inputEnvelope = 0.0;
    m_bandEnvelope = 0.0;
}

void StereoExciter::syncParameters()
{
    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    m_tune = value(Constants::NahdXml::xmlKeyTune(), 0.5f);
    m_peak = value(Constants::NahdXml::xmlKeyPeak(), 0.0f);
    m_zeroFill = value(Constants::NahdXml::xmlKeyZeroFill(), 0.0f);
    m_timbre = value(Constants::NahdXml::xmlKeyTimbre(), 0.5f);
    m_harmonics = value(Constants::NahdXml::xmlKeyHarmonics(), 0.0f);

    m_coefficientsDirty = true;
}

void StereoExciter::sync()
{
    syncParameters();
}

} // namespace noteahead
