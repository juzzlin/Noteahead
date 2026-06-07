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

#include "domain/effects/auto_panner_effect.hpp"

#include "common/constants.hpp"
#include "common/parameter_mapper.hpp"
#include "domain/dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

AutoPannerEffect::AutoPannerEffect()
{
    addParameter({ Constants::NahdXml::xmlKeyWaveform().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter({ Constants::NahdXml::xmlKeyIntensity().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyRate().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter({ Constants::NahdXml::xmlKeySync().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter({ Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString(), 0.25f, 0, 100, 25 });
}

std::string AutoPannerEffect::typeIdString()
{
    return "e5f6a7b8-c9d0-4e1f-2a3b-4c5d6e7f8a9b";
}

std::string AutoPannerEffect::type() const
{
    return Constants::RackEffectType::autoPanner().toStdString();
}

std::string AutoPannerEffect::typeId() const
{
    return typeIdString();
}

void AutoPannerEffect::process(double & left, double & right)
{
    const double lfoValue = m_lfo.nextSample(); // -1.0 to 1.0
    const double pan = 0.5 + (lfoValue * 0.5 * m_intensity); // 0.0 to 1.0
    const double gainL = std::min(1.0, 2.0 - pan * 2.0);
    const double gainR = std::min(1.0, pan * 2.0);

    left *= gainL;
    right *= gainR;
}

void AutoPannerEffect::process(AudioContext & context)
{
    m_lfo.setSampleRate(context.sampleRate);
    updateLfoFrequency();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void AutoPannerEffect::sync()
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyWaveform().toStdString()); p) {
        m_lfo.setWaveform(static_cast<Lfo::Waveform>(static_cast<int>(p->get().value() * 3.0f + 0.5f)));
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyIntensity().toStdString()); p) {
        m_intensity = static_cast<double>(p->get().value());
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeySync().toStdString()); p) {
        m_sync = p->get().value() > 0.5f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        m_rate = static_cast<double>(p->get().value());
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p) {
        m_syncDivision = static_cast<double>(p->get().value());
    }
    updateLfoFrequency();
}

void AutoPannerEffect::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    updateLfoFrequency();
}

void AutoPannerEffect::updateLfoFrequency()
{
    if (m_sync) {
        const double bps = static_cast<double>(bpm()) / 60.0;
        m_lfo.setFrequency(bps / (m_syncDivision * 4.0));
    } else {
        m_lfo.setFrequency(ParameterMapper::mapLfoFrequency(m_rate, 0.05, 20.0));
    }
}

} // namespace noteahead
