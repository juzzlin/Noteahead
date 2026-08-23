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

#include "monitor.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"
#include "../tracker/parameter.hpp"

namespace noteahead {

Monitor::Monitor()
{
    addParameter({ Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 4, 0, 1, Parameter::Type::Discrete });
}

std::string Monitor::typeIdString()
{
    return "37d7ace4-0bf8-4f85-afbc-7b8146656825";
}

std::string Monitor::type() const
{
    return Constants::RackEffectType::monitor().toStdString();
}

std::string Monitor::typeId() const
{
    return typeIdString();
}

Monitor::Mode Monitor::mode() const
{
    return m_mode;
}

void Monitor::processSample(double & left, double & right)
{
    switch (m_mode) {
    case Mode::Stereo:
        break;
    case Mode::Mono: {
        const double mono = (left + right) * 0.5;
        left = mono;
        right = mono;
        break;
    }
    case Mode::Left:
        right = left;
        break;
    case Mode::Right:
        left = right;
        break;
    case Mode::Side: {
        const double side = (left - right) * 0.5;
        left = side;
        right = side;
        break;
    }
    }
}

void Monitor::processBlock(AudioContext & context)
{
    // Stereo is the whole effect doing nothing, and an export is not listening, so neither is worth
    // walking the buffer for.
    if (context.offline || m_mode == Mode::Stereo) {
        return;
    }

    Effect::processBlock(context);
}

void Monitor::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        m_mode = static_cast<Mode>(p->get().xmlValue());
    }
}

} // namespace noteahead
