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

#ifndef DRUM_STRIKE_HPP
#define DRUM_STRIKE_HPP

#include <optional>

namespace noteahead {

class DrumAmpEnvelope;
class DrumEngine;

//! Strikes a drum engine that plays under a DrumAmpEnvelope.
//!
//! A hit that lands while the last one still rings makes the envelope walk the level down to zero
//! before its attack (see DrumAmpEnvelope::trigger()). Striking the engine at once would play the
//! new hit's attack into that fade and then jump back to full level, which is a click. So the
//! strike waits for the fade: the old hit fades out, and the new one starts from silence along
//! with the attack.
class DrumStrike
{
public:
    //! Struck now, or as soon as the envelope has faded what is still sounding.
    void strike(DrumEngine & engine, DrumAmpEnvelope & envelope, float velocity);
    //! Before each sample: strikes the engine once the fade it waits for is over.
    void advance(DrumEngine & engine, DrumAmpEnvelope & envelope);
    //! Strikes a waiting hit at once, for an envelope switched off before its fade was over.
    void strikeNow(DrumEngine & engine);
    void reset();
    bool isPending() const;

private:
    std::optional<float> m_pendingVelocity;
};

} // namespace noteahead

#endif // DRUM_STRIKE_HPP
