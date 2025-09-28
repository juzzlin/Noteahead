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

#ifndef COLUMN_SETTINGS_MODEL_HPP
#define COLUMN_SETTINGS_MODEL_HPP

#include <QObject>

#include "../../domain/column_settings.hpp"

namespace noteahead {

class ColumnSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 trackIndex READ trackIndex WRITE setTrackIndex NOTIFY trackIndexChanged)
    Q_PROPERTY(quint64 columnIndex READ columnIndex WRITE setColumnIndex NOTIFY columnIndexChanged)

    Q_PROPERTY(qint8 chordNote1Offset READ chordNote1Offset WRITE setChordNote1Offset NOTIFY chordNote1OffsetChanged)
    Q_PROPERTY(quint8 chordNote1Velocity READ chordNote1Velocity WRITE setChordNote1Velocity NOTIFY chordNote1VelocityChanged)
    Q_PROPERTY(qint8 chordNote2Offset READ chordNote2Offset WRITE setChordNote2Offset NOTIFY chordNote2OffsetChanged)
    Q_PROPERTY(quint8 chordNote2Velocity READ chordNote2Velocity WRITE setChordNote2Velocity NOTIFY chordNote2VelocityChanged)
    Q_PROPERTY(qint8 chordNote3Offset READ chordNote3Offset WRITE setChordNote3Offset NOTIFY chordNote3OffsetChanged)
    Q_PROPERTY(quint8 chordNote3Velocity READ chordNote3Velocity WRITE setChordNote3Velocity NOTIFY chordNote3VelocityChanged)

public:
    explicit ColumnSettingsModel(QObject * parent = nullptr);
    ~ColumnSettingsModel() override;

    Q_INVOKABLE void save();
    Q_INVOKABLE void requestData();
    Q_INVOKABLE void reset();

    void setColumnSettings(const ColumnSettings & settings);

    quint64 trackIndex() const;
    void setTrackIndex(quint64 trackIndex);

    quint64 columnIndex() const;
    void setColumnIndex(quint64 columnIndex);

    qint8 chordNote1Offset() const;
    void setChordNote1Offset(qint8 offset);

    quint8 chordNote1Velocity() const;
    void setChordNote1Velocity(quint8 velocity);

    qint8 chordNote2Offset() const;
    void setChordNote2Offset(qint8 offset);

    quint8 chordNote2Velocity() const;
    void setChordNote2Velocity(quint8 velocity);

    qint8 chordNote3Offset() const;
    void setChordNote3Offset(qint8 offset);

    quint8 chordNote3Velocity() const;
    void setChordNote3Velocity(quint8 velocity);

signals:
    void dataRequested();
    void dataReceived();

    void trackIndexChanged();
    void columnIndexChanged();

    void chordNote1OffsetChanged();
    void chordNote1VelocityChanged();
    void chordNote2OffsetChanged();
    void chordNote2VelocityChanged();
    void chordNote3OffsetChanged();
    void chordNote3VelocityChanged();

    void saveRequested(quint64 trackIndex, quint64 columnIndex, const ColumnSettings & settings);

private:
    quint64 m_trackIndex = 0;
    quint64 m_columnIndex = 0;
    ColumnSettings m_settings;
};

} // namespace noteahead

#endif // COLUMN_SETTINGS_MODEL_HPP
