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

#include "effect.hpp"

#include "../dsp/audio_context.hpp"

namespace noteahead {

Effect::~Effect() = default;

Effect::StringList Effect::parameterNames() const
{
    StringList names;
    for (const auto & [name, p] : parameters()) {
        names.push_back(name);
    }
    return names;
}

void Effect::process(AudioContext & context)
{
    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

bool Effect::enabled() const
{
    return m_enabled;
}

void Effect::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void Effect::reset()
{
}

void Effect::sync()
{
}

void Effect::setBpm(float)
{
}

} // namespace noteahead
