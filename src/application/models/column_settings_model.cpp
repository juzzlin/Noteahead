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

#include "column_settings_model.hpp"

namespace noteahead {

ColumnSettingsModel::ColumnSettingsModel(QObject * parent)
  : QObject(parent)
{
}

ColumnSettingsModel::~ColumnSettingsModel() = default;

void ColumnSettingsModel::save()
{
    emit saveRequested(m_trackIndex, m_columnIndex, m_settings);
}

void ColumnSettingsModel::reset()
{
    m_settings = {};

    emit chordNote1OffsetChanged();
    emit chordNote1VelocityChanged();
    emit chordNote2OffsetChanged();
    emit chordNote2VelocityChanged();
    emit chordNote3OffsetChanged();
    emit chordNote3VelocityChanged();
}

void ColumnSettingsModel::requestData()
{
    emit dataRequested();
}

void ColumnSettingsModel::setColumnSettings(const ColumnSettings & settings)
{
    const bool note1OffsetChanged = m_settings.chordAutomationSettings.note1.offset != settings.chordAutomationSettings.note1.offset;
    const bool note1VelocityChanged = m_settings.chordAutomationSettings.note1.velocity != settings.chordAutomationSettings.note1.velocity;
    const bool note2OffsetChanged = m_settings.chordAutomationSettings.note2.offset != settings.chordAutomationSettings.note2.offset;
    const bool note2VelocityChanged = m_settings.chordAutomationSettings.note2.velocity != settings.chordAutomationSettings.note2.velocity;
    const bool note3OffsetChanged = m_settings.chordAutomationSettings.note3.offset != settings.chordAutomationSettings.note3.offset;
    const bool note3VelocityChanged = m_settings.chordAutomationSettings.note3.velocity != settings.chordAutomationSettings.note3.velocity;

    m_settings = settings;

    if (note1OffsetChanged) {
        emit chordNote1OffsetChanged();
    }
    if (note1VelocityChanged) {
        emit chordNote1VelocityChanged();
    }
    if (note2OffsetChanged) {
        emit chordNote2OffsetChanged();
    }
    if (note2VelocityChanged) {
        emit chordNote2VelocityChanged();
    }
    if (note3OffsetChanged) {
        emit chordNote3OffsetChanged();
    }
    if (note3VelocityChanged) {
        emit chordNote3VelocityChanged();
    }

    emit dataReceived();
}

quint64 ColumnSettingsModel::trackIndex() const
{
    return m_trackIndex;
}

void ColumnSettingsModel::setTrackIndex(quint64 trackIndex)
{
    if (m_trackIndex != trackIndex) {
        m_trackIndex = trackIndex;
        emit trackIndexChanged();
    }
}

quint64 ColumnSettingsModel::columnIndex() const
{
    return m_columnIndex;
}

void ColumnSettingsModel::setColumnIndex(quint64 columnIndex)
{
    if (m_columnIndex != columnIndex) {
        m_columnIndex = columnIndex;
        emit columnIndexChanged();
    }
}

qint8 ColumnSettingsModel::chordNote1Offset() const
{
    return m_settings.chordAutomationSettings.note1.offset;
}

void ColumnSettingsModel::setChordNote1Offset(qint8 offset)
{
    if (m_settings.chordAutomationSettings.note1.offset != offset) {
        m_settings.chordAutomationSettings.note1.offset = offset;
        emit chordNote1OffsetChanged();
    }
}

quint8 ColumnSettingsModel::chordNote1Velocity() const
{
    return m_settings.chordAutomationSettings.note1.velocity;
}

void ColumnSettingsModel::setChordNote1Velocity(quint8 velocity)
{
    if (m_settings.chordAutomationSettings.note1.velocity != velocity) {
        m_settings.chordAutomationSettings.note1.velocity = velocity;
        emit chordNote1VelocityChanged();
    }
}

qint8 ColumnSettingsModel::chordNote2Offset() const
{
    return m_settings.chordAutomationSettings.note2.offset;
}

void ColumnSettingsModel::setChordNote2Offset(qint8 offset)
{
    if (m_settings.chordAutomationSettings.note2.offset != offset) {
        m_settings.chordAutomationSettings.note2.offset = offset;
        emit chordNote2OffsetChanged();
    }
}

quint8 ColumnSettingsModel::chordNote2Velocity() const
{
    return m_settings.chordAutomationSettings.note2.velocity;
}

void ColumnSettingsModel::setChordNote2Velocity(quint8 velocity)
{
    if (m_settings.chordAutomationSettings.note2.velocity != velocity) {
        m_settings.chordAutomationSettings.note2.velocity = velocity;
        emit chordNote2VelocityChanged();
    }
}

qint8 ColumnSettingsModel::chordNote3Offset() const
{
    return m_settings.chordAutomationSettings.note3.offset;
}

void ColumnSettingsModel::setChordNote3Offset(qint8 offset)
{
    if (m_settings.chordAutomationSettings.note3.offset != offset) {
        m_settings.chordAutomationSettings.note3.offset = offset;
        emit chordNote3OffsetChanged();
    }
}

quint8 ColumnSettingsModel::chordNote3Velocity() const
{
    return m_settings.chordAutomationSettings.note3.velocity;
}

void ColumnSettingsModel::setChordNote3Velocity(quint8 velocity)
{
    if (m_settings.chordAutomationSettings.note3.velocity != velocity) {
        m_settings.chordAutomationSettings.note3.velocity = velocity;
        emit chordNote3VelocityChanged();
    }
}

} // namespace noteahead
