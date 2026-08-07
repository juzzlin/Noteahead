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

//! How much of the gentle path Zero Fill can add.
constexpr double MaxFill = 1.0;

//! Drive into the shaper. Fixed: it is Harmonics that sets how much of the result is heard, and a
//! drive control on top of that would only be a second way of saying the same thing.
constexpr double ShaperDrive = 3.0;

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

double StereoExciter::shape(double value) const
{
    const double driven = value * ShaperDrive;

    // Odd-symmetric: returns 3rd, 5th and so on, which read as edge.
    const double odd = std::tanh(driven);

    // Asymmetric: the two halves of the wave meet different parts of the curve, which is what
    // returns even harmonics, and those read as warmth.
    const double even = driven >= 0.0 ? std::tanh(driven) : std::tanh(driven * 0.4) / 0.4;

    // Timbre runs odd at one end and even at the other, blending rather than switching.
    const double blend = static_cast<double>(m_timbre);
    return odd * (1.0 - blend) + even * blend;
}

void StereoExciter::processSample(double & left, double & right)
{
    updateFilters();

    if (m_harmonics <= 0.0f) {
        // The filters carry state, so they have to keep running even when nothing is being added.
        sideChain(m_steepL, m_gentleL, left);
        sideChain(m_steepR, m_gentleR, right);
        m_harmonicsDb = 0.0;
        return;
    }

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const double amount = static_cast<double>(m_harmonics) * MaxHarmonics;

    const double sideL = sideChain(m_steepL, m_gentleL, left);
    const double sideR = sideChain(m_steepR, m_gentleR, right);

    double harmonicL = 0.0;
    double harmonicR = 0.0;

    if (factor == 1) {
        harmonicL = shape(sideL);
        harmonicR = shape(sideR);
    } else {
        // Harmonics of a band this high land above Nyquist at the base rate and fold back down as
        // inharmonic tones, which is the opposite of what the effect is for.
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(sideL), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(sideR), highR.data(), factor);
        for (uint8_t k = 0; k < factor; k++) {
            highL[k] = static_cast<float>(shape(static_cast<double>(highL[k])));
            highR[k] = static_cast<float>(shape(static_cast<double>(highR[k])));
        }
        harmonicL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        harmonicR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
    }

    // The shaper returns the band it was given along with the harmonics it generated, so what is
    // added has to have the band itself taken back out: an exciter that also turned up the band it
    // works on would just be an equalizer with extra steps.
    //
    // What is subtracted is the band times the shaper's small-signal gain, which is its slope
    // through zero, not the shaped signal: subtracting that would take the harmonics with it and
    // leave silence.
    harmonicL -= sideL * ShaperDrive;
    harmonicR -= sideR * ShaperDrive;

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
    m_harmonicsDb = 0.0;
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
