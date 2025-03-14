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

#include <map>
#include <unordered_map>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class MixerService : public QObject
{
    Q_OBJECT

public:
    explicit MixerService(QObject * parent = nullptr);

    ~MixerService() override;

    Q_INVOKABLE void muteColumn(size_t trackIndex, size_t columnIndex, bool mute);
    Q_INVOKABLE void invertMutedColumns(size_t trackIndex, size_t columnIndex);
    Q_INVOKABLE bool shouldColumnPlay(size_t trackIndex, size_t columnIndex) const;
    Q_INVOKABLE void soloColumn(size_t trackIndex, size_t columnIndex, bool solo);
    Q_INVOKABLE bool isColumnMuted(size_t trackIndex, size_t columnIndex) const;
    Q_INVOKABLE bool isColumnSoloed(size_t trackIndex, size_t columnIndex) const;
    Q_INVOKABLE uint8_t columnVelocityScale(size_t trackIndex, size_t columnIndex) const;
    Q_INVOKABLE void setColumnVelocityScale(size_t trackIndex, size_t columnIndex, uint8_t scale);

    Q_INVOKABLE void muteTrack(size_t trackIndex, bool mute);
    Q_INVOKABLE void invertMutedTracks(size_t trackIndex);
    Q_INVOKABLE bool shouldTrackPlay(size_t trackIndex) const;
    Q_INVOKABLE void soloTrack(size_t trackIndex, bool solo);
    Q_INVOKABLE bool isTrackMuted(size_t trackIndex) const;
    Q_INVOKABLE bool isTrackSoloed(size_t trackIndex) const;
    Q_INVOKABLE uint8_t trackVelocityScale(size_t trackIndex) const;
    Q_INVOKABLE void setTrackVelocityScale(size_t trackIndex, uint8_t scale);

    Q_INVOKABLE uint8_t effectiveVelocity(size_t trackIndex, size_t columnIndex, uint8_t velocity) const;

    Q_INVOKABLE void update();

    void clear();

    void setColumnCount(size_t trackIndex, size_t count);
    using TrackIndexList = std::vector<size_t>;
    void setTrackIndices(TrackIndexList indices);

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

signals:
    void columnMuted(size_t trackIndex, size_t columnIndex, bool muted);
    void columnSoloed(size_t trackIndex, size_t columnIndex, bool soloed);
    void columnVelocityScaleChanged(size_t trackIndex, size_t columnIndex, uint8_t velocityScale);
    void columnCountOfTrackRequested(size_t trackIndex);

    void trackMuted(size_t trackIndex, bool muted);
    void trackSoloed(size_t trackIndex, bool soloed);
    void trackVelocityScaleChanged(size_t trackIndex, uint8_t velocityScale);
    void trackIndicesRequested();

    void cleared();
    void configurationChanged();

private:
    bool hasMutedColumns(size_t trackIndex) const;
    bool hasSoloedColumns(size_t trackIndex) const;
    bool hasMutedTracks() const;
    bool hasSoloedTracks() const;

    using TrackAndColumn = std::pair<size_t, size_t>;
    using TrackAndColumnMuteSoloMap = std::map<TrackAndColumn, bool>;
    TrackAndColumnMuteSoloMap m_mutedColumns;
    TrackAndColumnMuteSoloMap m_soloedColumns;

    using TrackAndColumnVelocityScaleMap = std::map<TrackAndColumn, uint8_t>;
    TrackAndColumnVelocityScaleMap m_columnVelocityScaleMap;

    using TrackMuteSoloMap = std::unordered_map<size_t, bool>;
    TrackMuteSoloMap m_mutedTracks;
    TrackMuteSoloMap m_soloedTracks;

    using TrackVelocityScaleMap = std::map<size_t, uint8_t>;
    TrackVelocityScaleMap m_trackVelocityScaleMap;

    using ColumnCountMap = std::map<size_t, size_t>;
    ColumnCountMap m_columnCountMap;
    TrackIndexList m_trackIndexList;
};

} // namespace noteahead

#endif // MIXER_SERVICE_HPP
