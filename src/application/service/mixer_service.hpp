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

    Q_INVOKABLE void muteColumn(quint64 trackIndex, quint64 columnIndex, bool mute);
    Q_INVOKABLE void invertMutedColumns(quint64 trackIndex, quint64 columnIndex);
    Q_INVOKABLE void invertSoloedColumns(quint64 trackIndex, quint64 columnIndex);
    Q_INVOKABLE bool shouldColumnPlay(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE void soloColumn(quint64 trackIndex, quint64 columnIndex, bool solo);
    Q_INVOKABLE bool isColumnMuted(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE bool isColumnSoloed(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE quint8 columnVelocityScale(quint64 trackIndex, quint64 columnIndex) const;
    Q_INVOKABLE void setColumnVelocityScale(quint64 trackIndex, quint64 columnIndex, quint8 scale);

    Q_INVOKABLE void muteTrack(quint64 trackIndex, bool mute);
    Q_INVOKABLE void invertMutedTracks(quint64 trackIndex);
    Q_INVOKABLE void invertSoloedTracks(quint64 trackIndex);
    Q_INVOKABLE bool shouldTrackPlay(quint64 trackIndex) const;
    Q_INVOKABLE void soloTrack(quint64 trackIndex, bool solo);
    Q_INVOKABLE bool isTrackMuted(quint64 trackIndex) const;
    Q_INVOKABLE bool isTrackSoloed(quint64 trackIndex) const;
    Q_INVOKABLE quint8 trackVelocityScale(quint64 trackIndex) const;
    Q_INVOKABLE void setTrackVelocityScale(quint64 trackIndex, quint8 scale);

    Q_INVOKABLE quint8 effectiveVelocity(quint64 trackIndex, quint64 columnIndex, quint8 velocity) const;

    Q_INVOKABLE void update();

    void clear();

    void setColumnCount(quint64 trackIndex, quint64 count);
    using TrackIndexList = std::vector<quint64>;
    void setTrackIndices(TrackIndexList indices);

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

signals:
    void columnMuted(quint64 trackIndex, quint64 columnIndex, bool muted);
    void columnSoloed(quint64 trackIndex, quint64 columnIndex, bool soloed);
    void columnVelocityScaleChanged(quint64 trackIndex, quint64 columnIndex, quint8 velocityScale);
    void columnCountOfTrackRequested(quint64 trackIndex);

    void trackMuted(quint64 trackIndex, bool muted);
    void trackSoloed(quint64 trackIndex, bool soloed);
    void trackVelocityScaleChanged(quint64 trackIndex, quint8 velocityScale);
    void trackIndicesRequested();

    void cleared();
    void configurationChanged();

private:
    bool hasMutedColumns(quint64 trackIndex) const;
    bool hasSoloedColumns(quint64 trackIndex) const;
    bool hasMutedTracks() const;
    bool hasSoloedTracks() const;

    using TrackAndColumn = std::pair<quint64, quint64>;
    using TrackAndColumnMuteSoloMap = std::map<TrackAndColumn, bool>;
    TrackAndColumnMuteSoloMap m_mutedColumns;
    TrackAndColumnMuteSoloMap m_soloedColumns;

    using TrackAndColumnVelocityScaleMap = std::map<TrackAndColumn, quint8>;
    TrackAndColumnVelocityScaleMap m_columnVelocityScaleMap;

    using TrackMuteSoloMap = std::unordered_map<quint64, bool>;
    TrackMuteSoloMap m_mutedTracks;
    TrackMuteSoloMap m_soloedTracks;

    using TrackVelocityScaleMap = std::map<quint64, quint8>;
    TrackVelocityScaleMap m_trackVelocityScaleMap;

    using ColumnCountMap = std::map<quint64, quint64>;
    ColumnCountMap m_columnCountMap;
    TrackIndexList m_trackIndexList;
};

} // namespace noteahead

#endif // MIXER_SERVICE_HPP
