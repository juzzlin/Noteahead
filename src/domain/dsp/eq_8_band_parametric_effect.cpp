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

#include "eq_8_band_parametric_effect.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

Eq8BandParametricEffect::Eq8BandParametricEffect()
{
    for (int i = 0; i < static_cast<int>(NumBands); i++) {
        addParameter(Parameter { Constants::NahdXml::xmlKeyEq8BandParametricType(i).toStdString(), 0.0f, 0, 6, 0 });
        addParameter(Parameter { Constants::NahdXml::xmlKeyEq8BandParametricFreq(i).toStdString(), 0.5f, 20, 20000, 1000 });
        addParameter(Parameter { Constants::NahdXml::xmlKeyEq8BandParametricGain(i).toStdString(), 0.5f, -24, 24, 0 });
        addParameter(Parameter { Constants::NahdXml::xmlKeyEq8BandParametricQ(i).toStdString(), 0.3f, 1, 100, 7, 10 });
    }

    syncParameters();
}

void Eq8BandParametricEffect::process(float & left, float & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    processStereo(left, right);
}

void Eq8BandParametricEffect::updateBuffers()
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

void Eq8BandParametricEffect::processStereo(float & left, float & right)
{
    for (auto & band : m_bands) {
        left = band.filterL.process(left);
        right = band.filterR.process(right);
    }
}

void Eq8BandParametricEffect::reset()
{
    for (auto & band : m_bands) {
        band.reset();
    }
}

void Eq8BandParametricEffect::sync()
{
    m_shouldUpdateBuffers = true;
}

void Eq8BandParametricEffect::syncParameters()
{
    for (int i = 0; i < static_cast<int>(NumBands); i++) {
        auto & band = m_bands[i];

        if (auto p = parameter(Constants::NahdXml::xmlKeyEq8BandParametricType(i).toStdString()); p) {
            band.type = static_cast<BiquadFilter::Type>(std::clamp(static_cast<int>(std::round(p->get().value() * 6.0f)), 0, 6));
        }

        if (auto p = parameter(Constants::NahdXml::xmlKeyEq8BandParametricFreq(i).toStdString()); p) {
            band.frequency = static_cast<float>(ParameterMapper::mapLogFrequency(p->get().value(), 20.0, 20000.0));
        }

        if (auto p = parameter(Constants::NahdXml::xmlKeyEq8BandParametricGain(i).toStdString()); p) {
            band.gainDb = -24.0f + p->get().value() * 48.0f;
        }

        if (auto p = parameter(Constants::NahdXml::xmlKeyEq8BandParametricQ(i).toStdString()); p) {
            band.q = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 0.1, 10.0));
        }

        band.updateCoefficients(m_sampleRate);
    }
}

} // namespace noteahead
