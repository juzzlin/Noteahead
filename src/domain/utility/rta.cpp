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

#include "rta.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"

#include <cmath>

namespace noteahead {

Rta::Rta()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyBandCount().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDbRange().toStdString(), 2.0f, 0, 3, 2, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyShowPinkNoise().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPinkNoiseLevel().toStdString(), -18.0f, -80, 0, -18, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySpeed().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFftRate().toStdString(), 1.0f, 0, 2, 1, 1, Parameter::Type::Discrete });

    Rta::syncParameters();
}

std::string Rta::typeIdString()
{
    return "b9f2e4d7-3a8c-4e6b-9f1d-2c5e7a0b3d4f";
}

std::string Rta::type() const
{
    return Constants::RackEffectType::rta().toStdString();
}

std::string Rta::typeId() const
{
    return typeIdString();
}

bool Rta::isSettled() const
{
    return !m_analysisEnabled.load(std::memory_order_relaxed);
}

void Rta::setAnalysisEnabled(bool enabled)
{
    m_analysisEnabled.store(enabled, std::memory_order_relaxed);
}

std::vector<float> Rta::bandMagnitudesDb() const
{
    return m_core.bandMagnitudesDb();
}

std::vector<std::pair<float, float>> Rta::bandLogPositions() const
{
    return m_core.bandLogPositions();
}

uint32_t Rta::layoutGeneration() const
{
    return m_core.layoutGeneration();
}

void Rta::processSample(double &, double &)
{
}

void Rta::processBlock(AudioContext & context)
{
    if (!m_analysisEnabled.load(std::memory_order_relaxed)) {
        return;
    }

    if (m_shouldSync) {
        syncParameters();
    }

    m_core.process(context);
}

void Rta::reset()
{
    m_core.reset();
}

void Rta::sync()
{
    m_shouldSync = true;
}

void Rta::syncParameters()
{
    m_shouldSync = false;

    const auto mode = [this](const QString & key) {
        if (const auto p = parameter(key.toStdString()); p) {
            return static_cast<int>(std::round(p->get().value()));
        }
        return 0;
    };
    m_core.configure(mode(Constants::NahdXml::xmlKeyBandCount()), mode(Constants::NahdXml::xmlKeySpeed()), mode(Constants::NahdXml::xmlKeyFftRate()));
}

} // namespace noteahead
