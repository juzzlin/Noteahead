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

#include "panner_effect.hpp"

#include "../../common/constants.hpp"

#include <algorithm>

namespace noteahead {

PannerEffect::PannerEffect()
{
    addParameter({ Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyReverbWidth().toStdString(), 1.0f, 0, 10000, 10000, 100 });
}

std::string PannerEffect::typeIdString()
{
    return "d4e5f6a7-b8c9-4d0e-1f2a-3b4c5d6e7f8a";
}

std::string PannerEffect::type() const
{
    return Constants::RackEffectType::panner().toStdString();
}

std::string PannerEffect::typeId() const
{
    return typeIdString();
}

void PannerEffect::process(double & left, double & right)
{
    // Stereo Width (Mid-Side processing)
    // Mid = (L + R) / 2
    // Side = (L - R) / 2
    // New L = Mid + Side * width
    // New R = Mid - Side * width

    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;

    left = mid + side * m_width;
    right = mid - side * m_width;

    // Panning (Linear)
    const double gainL = std::min(1.0, 2.0 - m_pan * 2.0);
    const double gainR = std::min(1.0, m_pan * 2.0);

    left *= gainL;
    right *= gainR;
}

void PannerEffect::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        m_pan = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyReverbWidth().toStdString()); p) {
        m_width = static_cast<double>(p->get().value());
    }
}

} // namespace noteahead
