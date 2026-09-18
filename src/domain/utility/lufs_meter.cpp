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

#include "lufs_meter.hpp"

#include "../../common/constants.hpp"

namespace noteahead {

LufsMeter::LufsMeter()
{
}

std::string LufsMeter::typeIdString()
{
    return Constants::RackEffectType::lufsMeter().toStdString();
}

std::string LufsMeter::type() const
{
    return typeIdString();
}

std::string LufsMeter::typeId() const
{
    return typeIdString();
}

void LufsMeter::setSampleRate(double sampleRate)
{
    Effect::setSampleRate(sampleRate);
    m_meter.setSampleRate(static_cast<uint32_t>(sampleRate));
}

void LufsMeter::processSample(double & left, double & right)
{
    // A meter, not a processor: the samples go on untouched.
    m_meter.processSample(left, right);
}

void LufsMeter::reset()
{
    m_meter.reset();
}

void LufsMeter::requestReset()
{
    m_meter.requestReset();
}

void LufsMeter::sync()
{
}

float LufsMeter::momentaryLufs() const
{
    return m_meter.momentaryLufs();
}

float LufsMeter::shortTermLufs() const
{
    return m_meter.shortTermLufs();
}

float LufsMeter::integratedLufs() const
{
    return m_meter.integratedLufs();
}

} // namespace noteahead
