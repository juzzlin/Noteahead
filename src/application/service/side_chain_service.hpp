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

#ifndef SIDE_CHAIN_SERVICE_HPP
#define SIDE_CHAIN_SERVICE_HPP

#include "../../domain/side_chain_settings.hpp"
#include "../../domain/song.hpp"

#include <QObject>

#include <map>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class SideChainService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)

    Q_PROPERTY(bool sideChainEnabled READ sideChainEnabled WRITE setSideChainEnabled NOTIFY sideChainEnabledChanged)
    Q_PROPERTY(quint8 sideChainSourceTrack READ sideChainSourceTrack WRITE setSideChainSourceTrack NOTIFY sideChainSourceTrackChanged)
    Q_PROPERTY(quint8 sideChainSourceColumn READ sideChainSourceColumn WRITE setSideChainSourceColumn NOTIFY sideChainSourceColumnChanged)
    Q_PROPERTY(int sideChainLookahead READ sideChainLookahead WRITE setSideChainLookahead NOTIFY sideChainLookaheadChanged)
    Q_PROPERTY(int sideChainRelease READ sideChainRelease WRITE setSideChainRelease NOTIFY sideChainReleaseChanged)

public:
    explicit SideChainService(QObject * parent = nullptr);

    Song::EventList renderToEvents(const Song & song, const Song::EventList & events, size_t endPosition);

    void setSettings(quint64 trackIndex, const SideChainSettings & settings);
    SideChainSettings settings(quint64 trackIndex) const;

    quint64 trackIndex() const;
    void setTrackIndex(quint64 trackIndex);

    bool sideChainEnabled() const;
    void setSideChainEnabled(bool enabled);
    quint8 sideChainSourceTrack() const;
    void setSideChainSourceTrack(quint8 trackIndex);
    quint8 sideChainSourceColumn() const;
    void setSideChainSourceColumn(quint8 columnIndex);
    int sideChainLookahead() const;
    void setSideChainLookahead(int lookahead);
    int sideChainRelease() const;
    void setSideChainRelease(int release);
    Q_INVOKABLE quint32 sideChainTargetCount() const;
    Q_INVOKABLE bool sideChainTargetEnabled(quint32 index) const;
    Q_INVOKABLE void setSideChainTargetEnabled(quint32 index, bool enabled);
    Q_INVOKABLE quint8 sideChainTargetController(quint32 index) const;
    Q_INVOKABLE void setSideChainTargetController(quint32 index, quint8 controller);
    Q_INVOKABLE quint8 sideChainTargetTargetValue(quint32 index) const;
    Q_INVOKABLE void setSideChainTargetTargetValue(quint32 index, quint8 value);
    Q_INVOKABLE quint8 sideChainTargetReleaseValue(quint32 index) const;
    Q_INVOKABLE void setSideChainTargetReleaseValue(quint32 index, quint8 value);

    void clear();

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

signals:
    void settingsChanged(quint64 trackIndex);
    void trackIndexChanged();

    void sideChainEnabledChanged();
    void sideChainSourceTrackChanged();
    void sideChainSourceColumnChanged();
    void sideChainLookaheadChanged();
    void sideChainReleaseChanged();

    void sideChainTargetChanged(quint32 index);

private:
    std::map<quint64, SideChainSettings> m_settings;
    quint64 m_trackIndex { 0 };
};

} // namespace noteahead

#endif // SIDE_CHAIN_SERVICE_HPP
