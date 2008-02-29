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

#include "all_pass_filter.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

AllPassFilter::AllPassFilter()
{
    // Log-mapped: internal [0,1] → mapLogFrequency(internal, 20, 20000). Default 100 Hz ≈ internal 0.101.
    addParameter(Parameter { Constants::NahdXml::xmlKeyAllPassFilterFrequency().toStdString(),
                             Parameter::xmlValueToInternal(2038, 20, 20000), 20, 20000, 2038, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAllPassFilterQ().toStdString(),
                             Parameter::xmlValueToInternal(707, 10, 1000), 10, 1000, 707, 1000 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAllPassFilterStages().toStdString(), 1.0f, 1, maxStages, 1, 1, Parameter::Type::Discrete });

    syncParameters();
}

std::string AllPassFilter::typeIdString()
{
    return Constants::RackEffectType::allPassFilter().toStdString();
}

std::string AllPassFilter::type() const
{
    return typeIdString();
}

std::string AllPassFilter::typeId() const
{
    return typeIdString();
}

void AllPassFilter::updateCoefficients()
{
    if (m_sampleRate <= 0) {
        return;
    }
    const double maxFreq = std::min(20000.0, m_sampleRate * 0.49);
    const double freq = std::clamp(static_cast<double>(m_frequency), 20.0, maxFreq);
    const double K = std::tan(std::numbers::pi * freq / m_sampleRate);
    const double Kk = K * K;
    const double q = std::max(static_cast<double>(m_q), 0.01);
    const double KoverQ = K / q;
    m_b2 = Kk + KoverQ + 1.0;
    m_b1 = 2.0 * (Kk - 1.0);
    m_b0 = Kk - KoverQ + 1.0;
}

void AllPassFilter::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAllPassFilterFrequency().toStdString()); p) {
        m_frequency = static_cast<float>(ParameterMapper::mapLogFrequency(p->get().value(), 20.0, 20000.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAllPassFilterQ().toStdString()); p) {
        m_q = static_cast<float>(p->get().xmlValue()) / static_cast<float>(p->get().xmlScale());
        m_q = std::max(m_q, 0.01f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAllPassFilterStages().toStdString()); p) {
        m_stages = std::clamp(p->get().xmlValue(), 1, maxStages);
    }
    updateCoefficients();
}

void AllPassFilter::sync()
{
    m_shouldSyncParameters = true;
}

void AllPassFilter::process(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }
    if (const auto sr = static_cast<uint32_t>(m_sampleRate); sr != m_lastSampleRate) {
        m_lastSampleRate = sr;
        updateCoefficients();
    }
    if (m_shouldSyncParameters) {
        m_shouldSyncParameters = false;
        syncParameters();
    }

    for (int i = 0; i < m_stages; i++) {
        auto & sL = m_stateL[static_cast<size_t>(i)];
        auto & sR = m_stateR[static_cast<size_t>(i)];

        const double outL = (m_b0 * left + m_b1 * sL[0] + m_b2 * sL[1] - m_b1 * sL[2] - m_b0 * sL[3]) / m_b2;
        sL[1] = sL[0];
        sL[0] = left;
        sL[3] = sL[2];
        sL[2] = outL;
        left = outL;

        const double outR = (m_b0 * right + m_b1 * sR[0] + m_b2 * sR[1] - m_b1 * sR[2] - m_b0 * sR[3]) / m_b2;
        sR[1] = sR[0];
        sR[0] = right;
        sR[3] = sR[2];
        sR[2] = outR;
        right = outR;

        // Denormal protection
        for (size_t idx = 0; idx < 4; ++idx) {
            if (std::abs(sL[idx]) < 1.0e-15) {
                sL[idx] = 0.0;
            }
            if (std::abs(sR[idx]) < 1.0e-15) {
                sR[idx] = 0.0;
            }
        }
    }

    // NaN protection
    if (std::isnan(left) || std::isnan(right)) {
        reset();
        left = 0.0;
        right = 0.0;
    }
}

void AllPassFilter::reset()
{
    m_stateL = {};
    m_stateR = {};
    Effect::reset();
}

} // namespace noteahead
