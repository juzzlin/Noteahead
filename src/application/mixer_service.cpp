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

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "MixerService";

MixerService::MixerService(QObject * parent)
  : QObject { parent }
{
}

void MixerService::muteColumn(size_t trackIndex, size_t columnIndex, bool mute)
{
    juzzlin::L(TAG).info() << "Muting column " << columnIndex << " on track " << trackIndex << ": " << mute;
    m_mutedColumns[{ trackIndex, columnIndex }] = mute;
    update();
}

void MixerService::invertMutedColumns(size_t trackIndex, size_t columnIndex)
{
    emit columnCountOfTrackRequested(trackIndex);

    if (!isColumnMuted(trackIndex, columnIndex)) {
        if (!hasMutedColumns(trackIndex)) {
            for (size_t targetColumnIndex = 0; targetColumnIndex < m_columnCountMap[trackIndex]; targetColumnIndex++) {
                muteColumn(trackIndex, targetColumnIndex, columnIndex != targetColumnIndex);
            }
        } else {
            for (size_t targetColumnIndex = 0; targetColumnIndex < m_columnCountMap[trackIndex]; targetColumnIndex++) {
                muteColumn(trackIndex, targetColumnIndex, false);
            }
        }
    } else {
        for (size_t targetColumnIndex = 0; targetColumnIndex < m_columnCountMap[trackIndex]; targetColumnIndex++) {
            muteColumn(trackIndex, targetColumnIndex, columnIndex != targetColumnIndex);
        }
    }
}

bool MixerService::shouldColumnPlay(size_t trackIndex, size_t columnIndex) const
{
    if (!shouldTrackPlay(trackIndex)) {
        return false;
    }

    if (hasSoloedColumns(trackIndex)) {
        return isColumnSoloed(trackIndex, columnIndex) && !isColumnMuted(trackIndex, columnIndex);
    } else {
        return !isColumnMuted(trackIndex, columnIndex);
    }
}

void MixerService::soloColumn(size_t trackIndex, size_t columnIndex, bool solo)
{
    juzzlin::L(TAG).info() << "Soloing column " << columnIndex << " on track " << trackIndex << ": " << solo;
    m_soloedColumns[{ trackIndex, columnIndex }] = solo;
    update();
}

bool MixerService::isColumnMuted(size_t trackIndex, size_t columnIndex) const
{
    return m_mutedColumns.contains({ trackIndex, columnIndex }) && m_mutedColumns.at({ trackIndex, columnIndex });
}

bool MixerService::isColumnSoloed(size_t trackIndex, size_t columnIndex) const
{
    return m_soloedColumns.contains({ trackIndex, columnIndex }) && m_soloedColumns.at({ trackIndex, columnIndex });
}

uint8_t MixerService::columnVelocityScale(size_t trackIndex, size_t columnIndex) const
{
    if (m_columnVelocityScaleMap.contains({ trackIndex, columnIndex })) {
        return m_columnVelocityScaleMap.at({ trackIndex, columnIndex });
    } else {
        return 100;
    }
}

void MixerService::setColumnVelocityScale(size_t trackIndex, size_t columnIndex, uint8_t scale)
{
    m_columnVelocityScaleMap[{ trackIndex, columnIndex }] = scale;
    update();
}

bool MixerService::hasMutedColumns(size_t trackIndex) const
{
    return std::ranges::any_of(m_mutedColumns, [trackIndex](const auto & pair) {
        return pair.first.first == trackIndex && pair.second;
    });
}

bool MixerService::hasSoloedColumns(size_t trackIndex) const
{
    return std::ranges::any_of(m_soloedColumns, [trackIndex](const auto & pair) {
        return pair.first.first == trackIndex && pair.second;
    });
}

void MixerService::muteTrack(size_t trackIndex, bool mute)
{
    juzzlin::L(TAG).info() << "Muting track " << trackIndex << ": " << mute;
    m_mutedTracks[trackIndex] = mute;
    update();
}

void MixerService::invertMutedTracks(size_t trackIndex)
{
    emit trackIndicesRequested();

    if (!isTrackMuted(trackIndex)) {
        if (!hasMutedTracks()) {
            for (auto && targetTrackIndex : m_trackIndexList) {
                muteTrack(targetTrackIndex, targetTrackIndex != trackIndex);
            }
        } else {
            for (auto && targetTrackIndex : m_trackIndexList) {
                muteTrack(targetTrackIndex, false);
            }
        }
    } else {
        for (auto && targetTrackIndex : m_trackIndexList) {
            muteTrack(targetTrackIndex, targetTrackIndex != trackIndex);
        }
    }
}

bool MixerService::hasMutedTracks() const
{
    return std::ranges::any_of(m_mutedTracks, [](const auto & pair) {
        return pair.second;
    });
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
    update();
}

bool MixerService::isTrackMuted(size_t trackIndex) const
{
    return m_mutedTracks.contains(trackIndex) && m_mutedTracks.at(trackIndex);
}

bool MixerService::isTrackSoloed(size_t trackIndex) const
{
    return m_soloedTracks.contains(trackIndex) && m_soloedTracks.at(trackIndex);
}

uint8_t MixerService::trackVelocityScale(size_t trackIndex) const
{
    if (m_trackVelocityScaleMap.contains(trackIndex)) {
        return m_trackVelocityScaleMap.at(trackIndex);
    } else {
        return 100;
    }
}

void MixerService::setTrackVelocityScale(size_t trackIndex, uint8_t scale)
{
    m_trackVelocityScaleMap[trackIndex] = scale;
    update();
}

uint8_t MixerService::effectiveVelocity(size_t trackIndex, size_t columnIndex, uint8_t velocity) const
{
    return trackVelocityScale(trackIndex) * columnVelocityScale(trackIndex, columnIndex) * velocity / (100 * 100);
}

void MixerService::update()
{
    for (auto && [trackIndex, state] : m_mutedTracks) {
        emit trackMuted(trackIndex, state);
    }

    for (auto && [trackIndex, state] : m_soloedTracks) {
        emit trackSoloed(trackIndex, state);
    }

    for (const auto & [key, state] : m_mutedColumns) {
        emit columnMuted(key.first, key.second, state);
    }

    for (const auto & [key, state] : m_soloedColumns) {
        emit columnSoloed(key.first, key.second, state);
    }

    for (auto && [key, value] : m_columnVelocityScaleMap) {
        emit columnVelocityScaleChanged(key.first, key.second, value);
    }

    for (auto && [trackIndex, value] : m_trackVelocityScaleMap) {
        emit trackVelocityScaleChanged(trackIndex, value);
    }

    emit configurationChanged();
}

void MixerService::clear()
{
    juzzlin::L(TAG).info() << "Clearing";

    m_mutedColumns.clear();
    m_soloedColumns.clear();
    m_mutedTracks.clear();
    m_soloedTracks.clear();

    m_columnVelocityScaleMap.clear();
    m_trackVelocityScaleMap.clear();

    emit cleared();
}

void MixerService::setColumnCount(size_t trackIndex, size_t count)
{
    m_columnCountMap[trackIndex] = count;
}

void MixerService::setTrackIndices(TrackIndexList indices)
{
    m_trackIndexList = indices;
}

void MixerService::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Mixer started";

    clear();

    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyMixer()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::xmlKeyColumnMuted())) {
                bool trackOk = false, columnOk = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyTrackAttr()).toUInt(&trackOk);
                const size_t columnIndex = reader.attributes().value(Constants::xmlKeyColumnAttr()).toUInt(&columnOk);
                if (trackOk && columnOk) {
                    m_mutedColumns[{ trackIndex, columnIndex }] = true;
                }
            } else if (!reader.name().compare(Constants::xmlKeyColumnSoloed())) {
                bool trackOk = false, columnOk = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyTrackAttr()).toUInt(&trackOk);
                const size_t columnIndex = reader.attributes().value(Constants::xmlKeyColumnAttr()).toUInt(&columnOk);
                if (trackOk && columnOk) {
                    m_soloedColumns[{ trackIndex, columnIndex }] = true;
                }
            } else if (!reader.name().compare(Constants::xmlKeyTrackMuted())) {
                bool ok = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyIndex()).toUInt(&ok);
                if (ok) {
                    m_mutedTracks[trackIndex] = true;
                }
            } else if (!reader.name().compare(Constants::xmlKeyTrackSoloed())) {
                bool ok = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyIndex()).toUInt(&ok);
                if (ok) {
                    m_soloedTracks[trackIndex] = true;
                }
            } else if (!reader.name().compare(Constants::xmlKeyColumnVelocityScale())) {
                bool trackOk = false, columnOk = false, valueOk = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyTrackAttr()).toUInt(&trackOk);
                const size_t columnIndex = reader.attributes().value(Constants::xmlKeyColumnAttr()).toUInt(&columnOk);
                const uint8_t value = static_cast<uint8_t>(reader.attributes().value(Constants::xmlKeyValue()).toUInt(&valueOk));
                if (trackOk && columnOk && valueOk) {
                    m_columnVelocityScaleMap[{ trackIndex, columnIndex }] = value;
                }
            } else if (!reader.name().compare(Constants::xmlKeyTrackVelocityScale())) {
                bool trackOk = false, valueOk = false;
                const size_t trackIndex = reader.attributes().value(Constants::xmlKeyIndex()).toUInt(&trackOk);
                const uint8_t value = static_cast<uint8_t>(reader.attributes().value(Constants::xmlKeyValue()).toUInt(&valueOk));
                if (trackOk && valueOk) {
                    m_trackVelocityScaleMap[trackIndex] = value;
                }
            }
        }
        reader.readNext();
    }

    if (reader.hasError()) {
        juzzlin::L(TAG).error() << "XML parsing error: " << reader.errorString().toStdString();
    }

    juzzlin::L(TAG).trace() << "Reading Mixer ended";
}

void MixerService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyMixer());

    for (const auto & [key, state] : m_mutedColumns) {
        if (state) {
            writer.writeStartElement(Constants::xmlKeyColumnMuted());
            writer.writeAttribute(Constants::xmlKeyTrackAttr(), QString::number(key.first));
            writer.writeAttribute(Constants::xmlKeyColumnAttr(), QString::number(key.second));
            writer.writeEndElement();
        }
    }

    for (const auto & [key, state] : m_soloedColumns) {
        if (state) {
            writer.writeStartElement(Constants::xmlKeyColumnSoloed());
            writer.writeAttribute(Constants::xmlKeyTrackAttr(), QString::number(key.first));
            writer.writeAttribute(Constants::xmlKeyColumnAttr(), QString::number(key.second));
            writer.writeEndElement();
        }
    }

    for (auto && [trackIndex, state] : m_mutedTracks) {
        if (state) {
            writer.writeStartElement(Constants::xmlKeyTrackMuted());
            writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(trackIndex));
            writer.writeEndElement();
        }
    }

    for (auto && [trackIndex, state] : m_soloedTracks) {
        if (state) {
            writer.writeStartElement(Constants::xmlKeyTrackSoloed());
            writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(trackIndex));
            writer.writeEndElement();
        }
    }

    for (auto && [key, value] : m_columnVelocityScaleMap) {
        writer.writeStartElement(Constants::xmlKeyColumnVelocityScale());
        writer.writeAttribute(Constants::xmlKeyTrackAttr(), QString::number(key.first));
        writer.writeAttribute(Constants::xmlKeyColumnAttr(), QString::number(key.second));
        writer.writeAttribute(Constants::xmlKeyValue(), QString::number(value));
        writer.writeEndElement();
    }

    for (auto && [trackIndex, value] : m_trackVelocityScaleMap) {
        writer.writeStartElement(Constants::xmlKeyTrackVelocityScale());
        writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(trackIndex));
        writer.writeAttribute(Constants::xmlKeyValue(), QString::number(value));
        writer.writeEndElement();
    }

    writer.writeEndElement(); // Mixer
}

MixerService::~MixerService() = default;

} // namespace noteahead
