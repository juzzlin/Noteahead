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

#include "dimension.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <cmath>

namespace noteahead {

namespace {

//! How far apart the two copies can be taken. Past the top of this the pair stops reading as one
//! source made larger and starts reading as two sources, which is a chorus and not this effect.
constexpr double maxDetuneCents = 25.0;

//! Range the Low Cut sweeps.
constexpr double minLowCutHz = 20.0;
constexpr double maxLowCutHz = 500.0;

//! Butterworth, because this is a corner rather than a resonance.
constexpr double lowCutQ = 0.707;

//! How much generated side is added at the top of the Amount control. Past this the pair starts to
//! be louder than the centre it was made from.
constexpr double maxAmount = 1.2;

} // namespace

Dimension::Dimension()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyDetune().toStdString(), 0.28f, 0, 2500, 700, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmount().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(ParameterMapper::unmapLogFrequency(120.0, minLowCutHz, maxLowCutHz)), 20, 500, 120, 1, Parameter::Type::Continuous });

    // What this adds is a side signal and nothing else, so solo has something exact to isolate.
    addSoloParameter();

    syncParameters();
}

std::string Dimension::typeIdString()
{
    return "b4f0c3a7-6d19-4e82-95cb-1f27a8e04d63";
}

std::string Dimension::type() const
{
    return Constants::RackEffectType::dimension().toStdString();
}

std::string Dimension::typeId() const
{
    return typeIdString();
}

void Dimension::updateState()
{
    if (m_shouldSyncParameters || std::abs(m_sampleRate - m_lastSampleRate) >= 0.1) {
        syncParameters();
        m_lastSampleRate = m_sampleRate;
        m_shouldSyncParameters = false;
    }
}

void Dimension::syncParameters()
{
    const auto value = [this](const QString & key, float fallback) {
        const auto parameter = this->parameter(key.toStdString());
        return parameter ? parameter->get().value() : fallback;
    };

    const double sampleRate = m_sampleRate > 0 ? m_sampleRate : 48000.0;

    m_detuneCents = static_cast<float>(value(Constants::NahdXml::xmlKeyDetune(), 0.28f) * maxDetuneCents);
    m_amount = value(Constants::NahdXml::xmlKeyAmount(), 0.0f);
    m_lowCutHz = static_cast<float>(ParameterMapper::mapLogFrequency(value(Constants::NahdXml::xmlKeyHpfCutoff(), 0.5f), minLowCutHz, maxLowCutHz));

    m_shifterUp.setSampleRate(sampleRate);
    m_shifterDown.setSampleRate(sampleRate);
    m_shifterUp.setCents(m_detuneCents);
    m_shifterDown.setCents(-m_detuneCents);

    m_lowCut.calculateLowCut(m_lowCutHz, sampleRate, lowCutQ);
}

void Dimension::processSample(double & left, double & right)
{
    updateState();

    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;

    const double source = m_lowCut.process(mid);

    // The shifters run whatever the Amount, so that turning it up starts from lines that are
    // already full rather than from silence.
    const double up = m_shifterUp.process(source);
    const double down = m_shifterDown.process(source);

    // The difference, so that the two copies appear only in the side signal and with opposite
    // signs. That is what makes the mono sum below exactly the mid that came in.
    const double generated = (up - down) * 0.5;

    const double widened = side + generated * static_cast<double>(m_amount) * maxAmount;

    left = mid + widened;
    right = mid - widened;
}

void Dimension::reset()
{
    m_shifterUp.reset();
    m_shifterDown.reset();
    m_lowCut.reset();
}

void Dimension::sync()
{
    m_shouldSyncParameters = true;
}

} // namespace noteahead
