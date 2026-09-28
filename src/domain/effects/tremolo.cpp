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

#include "tremolo.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

Tremolo::Tremolo()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyWaveform().toStdString(), 3.0f, 0, 4, 3, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyIntensity().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 100, 25 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRateDivider().toStdString(), 1.0f, 1, maxRateDivider(), 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStereoPhase().toStdString(), 0.0f, 0, 180, 0 });
}

int Tremolo::maxRateDivider()
{
    return 64;
}

std::string Tremolo::typeIdString()
{
    return "b8c9d0e1-f2a3-4b4c-6d7e-8f9a0b1c2d3e";
}

std::string Tremolo::type() const
{
    return Constants::RackEffectType::tremolo().toStdString();
}

std::string Tremolo::typeId() const
{
    return typeIdString();
}

void Tremolo::processSample(double & left, double & right)
{
    // Downwards only: the LFO's bipolar swing is folded to 0..1 and taken away from unity, so the
    // loudest the tremolo ever is equals the signal that went in. Modulating either side of unity
    // instead makes turning the depth up turn the track up, which is heard as the effect being
    // louder rather than deeper.
    const double depthLeft = (1.0 - m_lfoLeft.nextSample()) * 0.5;
    const double depthRight = (1.0 - m_lfoRight.nextSample()) * 0.5;

    left *= 1.0 - m_intensity * depthLeft;
    right *= 1.0 - m_intensity * depthRight;
}

void Tremolo::processBlock(AudioContext & context)
{
    if (m_sampleRate != context.sampleRate) {
        m_sampleRate = context.sampleRate;
        m_lfoLeft.setSampleRate(context.sampleRate);
        m_lfoRight.setSampleRate(context.sampleRate);
        updateLfoFrequency();
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Tremolo::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p) {
        const auto waveform = static_cast<Lfo::Waveform>(std::clamp(p->get().xmlValue(), 0, 4));
        m_lfoLeft.setWaveform(waveform);
        m_lfoRight.setWaveform(waveform);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyIntensity().toStdString()); p) {
        m_intensity = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        m_rate = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySync().toStdString()); p) {
        m_sync = p->get().value() > 0.5f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) {
        m_syncDivision = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString()); p) {
        m_rateDivider = std::clamp(p->get().xmlValue(), 1, maxRateDivider());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStereoPhase().toStdString()); p) {
        m_stereoPhase = static_cast<double>(p->get().xmlValue());
        m_lfoRight.setPhase(m_stereoPhase / 360.0);
    }
    updateLfoFrequency();
}

void Tremolo::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    updateLfoFrequency();
}

void Tremolo::reset()
{
    Effect::reset();
    m_lfoLeft.reset();
    m_lfoRight.reset();
    m_lfoRight.setPhase(m_stereoPhase / 360.0);
}

void Tremolo::updateLfoFrequency()
{
    const double frequency = m_sync
      ? (static_cast<double>(bpm()) / 60.0) / (m_syncDivision * 4.0)
      : ParameterMapper::mapLfoFrequency(m_rate, 0.05, 20.0);
    m_lfoLeft.setFrequency(frequency / m_rateDivider);
    m_lfoRight.setFrequency(frequency / m_rateDivider);
}

} // namespace noteahead
