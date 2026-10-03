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

#include "drum_strike.hpp"

#include "drum_amp_envelope.hpp"
#include "drum_engine.hpp"

namespace noteahead {

void DrumStrike::strike(DrumEngine & engine, DrumAmpEnvelope & envelope, float velocity)
{
    // Nothing sounding: no fade to wait for, whatever level the envelope was left at
    if (!engine.isActive()) {
        envelope.reset();
    }
    envelope.trigger();
    if (envelope.state() == DrumAmpEnvelope::State::Choke) {
        m_pendingVelocity = velocity;
    } else {
        m_pendingVelocity.reset();
        engine.trigger(velocity);
    }
}

void DrumStrike::advance(DrumEngine & engine, DrumAmpEnvelope & envelope)
{
    if (!m_pendingVelocity) {
        return;
    }
    if (envelope.state() == DrumAmpEnvelope::State::Choke) {
        // The tail ran out during the fade: the attack starts now rather than after a fade of silence
        if (engine.isActive()) {
            return;
        }
        envelope.reset();
        envelope.trigger();
    }
    engine.trigger(*m_pendingVelocity);
    m_pendingVelocity.reset();
}

void DrumStrike::strikeNow(DrumEngine & engine)
{
    if (m_pendingVelocity) {
        engine.trigger(*m_pendingVelocity);
        m_pendingVelocity.reset();
    }
}

void DrumStrike::reset()
{
    m_pendingVelocity.reset();
}

bool DrumStrike::isPending() const
{
    return m_pendingVelocity.has_value();
}

} // namespace noteahead
