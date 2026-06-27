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
#include "../tracker/parameter.hpp"

namespace noteahead {

PannerEffect::PannerEffect()
{
    addParameter({ Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyWidth().toStdString(), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous, { "reverbWidth" } });
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
    m_panner.process(left, right);
}

void PannerEffect::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        m_panner.setPan(static_cast<double>(p->get().value()));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWidth().toStdString()); p) {
        m_panner.setWidth(static_cast<double>(p->get().value()));
    }
}

} // namespace noteahead
