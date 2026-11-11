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

#include "compressor_2a.hpp"

#include "../../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double minGainDb { -20.0 };
//! As much make-up as the unit's own output amplifier has: about 40 dB.
constexpr double maxGainDb { 40.0 };
//! The second harmonic the valve stage adds: about -52 dB at a device's usual level, -32 dB at
//! -6 dBFS.
constexpr double valveEvenOrder { 0.1 };

std::string key(const QString & name)
{
    return name.toStdString();
}

} // namespace

Compressor2A::Compressor2A()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    // Audible from the start, but light: about 1 dB held down at a device's usual level
    addParameter(Parameter { key(C::xmlKeyPeakReduction()), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { key(C::xmlKeyGain()), gainPosition(0.0), static_cast<int>(minGainDb * 100.0), static_cast<int>(maxGainDb * 100.0), 0, 100 });
    addParameter(Parameter { key(C::xmlKeyMode()), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addMixParameter(1.0f, MixLaw::Crossfade, 0, 10000, 100);
    addSoloParameter();

    setParameterDisplay(key(C::xmlKeyPeakReduction()), { .mapping = Display::Mapping::Value, .unit = "", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Peak Reduction") });
    setParameterDisplay(key(C::xmlKeyGain()), { .mapping = Display::Mapping::Value, .unit = "dB", .min = minGainDb, .max = maxGainDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Gain") });

    syncParameters();
}

std::string Compressor2A::typeIdString()
{
    return "8b9b81b9-5ec0-406a-80d5-c0db2baf5acb";
}

std::string Compressor2A::type() const
{
    return Constants::RackEffectType::compressor2A().toStdString();
}

std::string Compressor2A::typeId() const
{
    return typeIdString();
}

double Compressor2A::gainDb(float position)
{
    return minGainDb + std::clamp(static_cast<double>(position), 0.0, 1.0) * (maxGainDb - minGainDb);
}

float Compressor2A::gainPosition(double db)
{
    return static_cast<float>(std::clamp((db - minGainDb) / (maxGainDb - minGainDb), 0.0, 1.0));
}

double Compressor2A::valve(double value)
{
    return value + valveEvenOrder * value * value;
}

void Compressor2A::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : 0.0f;
    };
    m_core.setPeakReduction(static_cast<double>(valueOf(C::xmlKeyPeakReduction())));
    m_core.setLimit(static_cast<Mode>(std::lround(valueOf(C::xmlKeyMode()))) == Mode::Limit);
    m_gain = std::pow(10.0, gainDb(valueOf(C::xmlKeyGain())) / 20.0);
}

void Compressor2A::processSample(double & left, double & right)
{
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    m_core.setSampleRate(m_sampleRate);
    m_dcBlockerLeft.setSampleRate(m_sampleRate);
    m_dcBlockerRight.setSampleRate(m_sampleRate);

    const double gain = std::pow(10.0, m_core.processGainDb(left, right) / 20.0) * m_gain;
    // The valve's even harmonic comes with an offset, which the coupling capacitor takes back out
    left = m_dcBlockerLeft.process(valve(left * gain));
    right = m_dcBlockerRight.process(valve(right * gain));
}

void Compressor2A::reset()
{
    m_core.reset();
    m_dcBlockerLeft.reset();
    m_dcBlockerRight.reset();
}

void Compressor2A::sync()
{
    m_shouldSyncParameters = true;
}

bool Compressor2A::isSettled() const
{
    return m_core.isSettled();
}

float Compressor2A::reductionDb() const
{
    return static_cast<float>(m_core.reductionDb());
}

const EffectPresetList & Compressor2A::factoryPresets() const
{
    namespace C = Constants::NahdXml;
    const auto patch = [](float peakReduction, double gain, Mode mode, float mix) {
        return std::map<std::string, float> {
            { key(C::xmlKeyPeakReduction()), peakReduction },
            { key(C::xmlKeyGain()), gainPosition(gain) },
            { key(C::xmlKeyMode()), static_cast<float>(mode) },
            { key(C::xmlKeyMix()), mix },
        };
    };
    // Init first, as the way back; the rest alphabetical
    static const EffectPresetList presets {
        { "Init", {} },
        // Evens out a bass line without taking the attack off its notes
        { "Bass Leveler", patch(0.5f, 5.0, Mode::Compress, 1.0f) },
        // Squashed hard and blended in under the dry drums
        { "Drum Crush", patch(0.75f, 10.0, Mode::Limit, 0.4f) },
        // A light hold on a group or the mix, barely moving
        { "Gentle Glue", patch(0.3f, 1.0, Mode::Compress, 1.0f) },
        // Rides a pad or a string part slowly, as the cell lets go over seconds
        { "Pad Rider", patch(0.55f, 5.0, Mode::Compress, 1.0f) },
        // The classic: a lead held steady in front of the mix
        { "Vocal Leveler", patch(0.6f, 7.0, Mode::Compress, 1.0f) },
    };
    return presets;
}

std::vector<std::string> Compressor2A::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyPeakReduction().toStdString(),
        NahdXml::xmlKeyGain().toStdString(),
        NahdXml::xmlKeyMix().toStdString()
    };
}

} // namespace noteahead
