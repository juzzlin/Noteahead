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

#ifndef MIXER_SERVICE_HPP
#define MIXER_SERVICE_HPP

#include <QObject>

#include <unordered_map>

namespace noteahead {

class MixerService : public QObject
{
    Q_OBJECT
public:
    explicit MixerService(QObject * parent = nullptr);

    Q_INVOKABLE void muteTrack(size_t trackIndex, bool mute);

    Q_INVOKABLE bool shouldTrackPlay(size_t trackIndex) const;

    Q_INVOKABLE void soloTrack(size_t trackIndex, bool solo);

    Q_INVOKABLE bool isTrackMuted(size_t trackIndex) const;

    Q_INVOKABLE bool isTrackSoloed(size_t trackIndex) const;

signals:

    void trackMuted(size_t trackIndex, bool muted);

    void trackSoloed(size_t trackIndex, bool muted);

private:
    bool hasSoloedTracks() const;

    void updateTrackStates();

    std::unordered_map<size_t, bool> m_mutedTracks;

    std::unordered_map<size_t, bool> m_soloedTracks;
};

} // namespace noteahead

#endif // MIXER_SERVICE_HPP
