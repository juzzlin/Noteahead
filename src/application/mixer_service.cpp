// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "mixer_service.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

static const auto TAG = "MixerService";

MixerService::MixerService(QObject * parent)
  : QObject { parent }
{
}

void MixerService::muteTrack(size_t trackIndex, bool mute)
{
    juzzlin::L(TAG).info() << "Muting track " << trackIndex << ": " << mute;

    m_mutedTracks[trackIndex] = mute;

    updateTrackStates();
}

bool MixerService::hasSoloedTracks() const
{
    return std::ranges::any_of(m_soloedTracks, [](const auto & pair) {
        return pair.second;
    });
}

bool MixerService::shouldTrackPlay(size_t trackIndex) const
{
    if (hasSoloedTracks()) {
        return isTrackSoloed(trackIndex) && !isTrackMuted(trackIndex);
    } else {
        return !isTrackMuted(trackIndex);
    }
}

void MixerService::soloTrack(size_t trackIndex, bool solo)
{
    juzzlin::L(TAG).info() << "Soloing track " << trackIndex << ": " << solo;

    m_soloedTracks[trackIndex] = solo;

    updateTrackStates();
}

bool MixerService::isTrackMuted(size_t trackIndex) const
{
    return m_mutedTracks.contains(trackIndex) && m_mutedTracks.at(trackIndex);
}

bool MixerService::isTrackSoloed(size_t trackIndex) const
{
    return m_soloedTracks.contains(trackIndex) && m_soloedTracks.at(trackIndex);
}

void MixerService::updateTrackStates()
{
    for (auto && [trackIndex, state] : m_mutedTracks) {
        emit trackMuted(trackIndex, state);
    }

    for (auto && [trackIndex, state] : m_soloedTracks) {
        emit trackSoloed(trackIndex, state);
    }
}

} // namespace noteahead
