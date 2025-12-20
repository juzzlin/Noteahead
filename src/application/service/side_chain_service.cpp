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

#include "side_chain_service.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/note_data.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>

namespace noteahead {

static const auto TAG = "MidiSideChainService";

SideChainService::SideChainService(QObject * parent)
  : QObject { parent }
{
}

Song::EventList SideChainService::renderToEvents(const Song & song, const Song::EventList & events, size_t startPosition, size_t endPosition)
{
    Song::EventList processedEvents { events };
    Song::EventList sideChainEvents;

    const auto startTick = song.positionToTick(startPosition);
    const auto maxTick = song.positionToTick(endPosition);
    const double msPerTick = 60'000.0 / static_cast<double>(song.beatsPerMinute() * song.linesPerBeat() * song.ticksPerLine());

    for (auto const& [trackIndex, sideChainSettings] : m_settings) {
        if (!song.hasTrack(trackIndex)) {
            continue;
        }
        if (sideChainSettings.enabled) {
            juzzlin::L(TAG).debug() << "Side-chain enabled on track " << trackIndex;
            for (const auto & event : processedEvents) {
                if (const auto noteData = event->noteData(); noteData && noteData->type() == NoteData::Type::NoteOn) {
                    if (noteData->track() == sideChainSettings.sourceTrackIndex && noteData->column() == sideChainSettings.sourceColumnIndex) {

                        const auto addTargetEvents = [&](const SideChainSettings::Target & target) {
                            if (target.enabled) {
                                const auto lookaheadTicks = static_cast<size_t>(sideChainSettings.lookahead.count() / msPerTick);
                                const size_t proposedTargetTick = event->tick() > lookaheadTicks ? event->tick() - lookaheadTicks : 0;
                                const size_t targetTick = std::max(startTick, proposedTargetTick);
                                const auto targetEvent = std::make_shared<Event>(targetTick, MidiCcData(trackIndex, sideChainSettings.sourceColumnIndex, target.controller, target.targetValue));
                                sideChainEvents.push_back(targetEvent);

                                const auto releaseTicks = static_cast<size_t>(sideChainSettings.release.count() / msPerTick);
                                size_t releaseTick = event->tick() + releaseTicks;
                                if (releaseTick >= maxTick) {
                                    releaseTick = maxTick > 0 ? maxTick - 1 : 0;
                                }

                                const auto releaseEvent = std::make_shared<Event>(releaseTick, MidiCcData(trackIndex, sideChainSettings.sourceColumnIndex, target.controller, target.releaseValue));
                                sideChainEvents.push_back(releaseEvent);
                            }
                        };

                        for (const auto & target : sideChainSettings.targets) {
                            addTargetEvents(target);
                        }
                    }
                }
            }
        }
    }

    processedEvents.insert(processedEvents.end(), sideChainEvents.begin(), sideChainEvents.end());
    std::sort(processedEvents.begin(), processedEvents.end(), [](const auto & a, const auto & b) {
        return a->tick() < b->tick();
    });
    return processedEvents;
}

void SideChainService::setSettings(quint64 trackIndex, const SideChainSettings & settings)
{
    m_settings[trackIndex] = settings;
    emit settingsChanged(trackIndex);
}

SideChainSettings SideChainService::settings(quint64 trackIndex) const
{
    if (m_settings.count(trackIndex)) {
        return m_settings.at(trackIndex);
    }
    return {};
}

void SideChainService::clear()
{
    m_settings.clear();
}

void SideChainService::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading SideChain started";

    clear();

    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeySideChain()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeySideChainSettings())) {
            bool trackOk = false;
            const quint64 trackIndex = reader.attributes().value(Constants::NahdXml::xmlKeyTrackAttr()).toUInt(&trackOk);
            if (trackOk) {
                SideChainSettings settings;
                settings.enabled = *Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled());
                settings.sourceTrackIndex = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeySourceTrack());
                settings.sourceColumnIndex = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeySourceColumn());
                settings.lookahead = std::chrono::milliseconds(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyLookahead()));
                settings.release = std::chrono::milliseconds(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyRelease()));
                reader.readNext();

                while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeySideChainSettings()))) {
                    if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeySideChainTarget())) {
                        SideChainSettings::Target target;
                        target.enabled = *Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled());
                        target.controller = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyController());
                        target.targetValue = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyTargetValue());
                        target.releaseValue = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyReleaseValue());
                        settings.targets.push_back(target);
                        reader.skipCurrentElement();
                    } else {
                        reader.readNext();
                    }
                }
                m_settings[trackIndex] = settings;
            }
        }
        reader.readNext();
    }

    if (reader.hasError()) {
        juzzlin::L(TAG).error() << "XML parsing error: " << reader.errorString().toStdString();
    }

    juzzlin::L(TAG).trace() << "Reading SideChain ended";
}

void SideChainService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeySideChain());

    for (auto const& [trackIndex, settings] : m_settings) {
        writer.writeStartElement(Constants::NahdXml::xmlKeySideChainSettings());
        writer.writeAttribute(Constants::NahdXml::xmlKeyTrackAttr(), QString::number(trackIndex));
        writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), settings.enabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
        writer.writeAttribute(Constants::NahdXml::xmlKeySourceTrack(), QString::number(settings.sourceTrackIndex));
        writer.writeAttribute(Constants::NahdXml::xmlKeySourceColumn(), QString::number(settings.sourceColumnIndex));
        writer.writeAttribute(Constants::NahdXml::xmlKeyLookahead(), QString::number(settings.lookahead.count()));
        writer.writeAttribute(Constants::NahdXml::xmlKeyRelease(), QString::number(settings.release.count()));

        for(size_t i = 0; i < settings.targets.size(); ++i) {
            const auto& target = settings.targets[i];
            writer.writeStartElement(Constants::NahdXml::xmlKeySideChainTarget());
            writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), target.enabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
            writer.writeAttribute(Constants::NahdXml::xmlKeyController(), QString::number(target.controller));
            writer.writeAttribute(Constants::NahdXml::xmlKeyTargetValue(), QString::number(target.targetValue));
            writer.writeAttribute(Constants::NahdXml::xmlKeyReleaseValue(), QString::number(target.releaseValue));
            writer.writeEndElement();
        }

        writer.writeEndElement(); // SideChainTrackSettings
    }

    writer.writeEndElement(); // SideChain
}

quint64 SideChainService::trackIndex() const
{
    return m_trackIndex;
}

void SideChainService::setTrackIndex(quint64 trackIndex)
{
    if (m_trackIndex != trackIndex) {
        m_trackIndex = trackIndex;
        emit trackIndexChanged();
    }
}

bool SideChainService::sideChainEnabled() const
{
    return settings(m_trackIndex).enabled;
}

void SideChainService::setSideChainEnabled(bool enabled)
{
    if (sideChainEnabled() != enabled) {
        auto currentSettings = settings(m_trackIndex);
        currentSettings.enabled = enabled;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainEnabledChanged();
    }
}

quint8 SideChainService::sideChainSourceTrack() const
{
    return settings(m_trackIndex).sourceTrackIndex;
}

void SideChainService::setSideChainSourceTrack(quint8 trackIndex)
{
    if (sideChainSourceTrack() != trackIndex) {
        auto currentSettings = settings(m_trackIndex);
        currentSettings.sourceTrackIndex = trackIndex;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainSourceTrackChanged();
    }
}

quint8 SideChainService::sideChainSourceColumn() const
{
    return settings(m_trackIndex).sourceColumnIndex;
}

void SideChainService::setSideChainSourceColumn(quint8 columnIndex)
{
    if (sideChainSourceColumn() != columnIndex) {
        auto currentSettings = settings(m_trackIndex);
        currentSettings.sourceColumnIndex = columnIndex;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainSourceColumnChanged();
    }
}

int SideChainService::sideChainLookahead() const
{
    return static_cast<int>(settings(m_trackIndex).lookahead.count());
}

void SideChainService::setSideChainLookahead(int lookahead)
{
    if (sideChainLookahead() != lookahead) {
        auto currentSettings = settings(m_trackIndex);
        currentSettings.lookahead = std::chrono::milliseconds(lookahead);
        setSettings(m_trackIndex, currentSettings);
        emit sideChainLookaheadChanged();
    }
}

int SideChainService::sideChainRelease() const
{
    return static_cast<int>(settings(m_trackIndex).release.count());
}

void SideChainService::setSideChainRelease(int release)
{
    if (sideChainRelease() != release) {
        auto currentSettings = settings(m_trackIndex);
        currentSettings.release = std::chrono::milliseconds(release);
        setSettings(m_trackIndex, currentSettings);
        emit sideChainReleaseChanged();
    }
}

quint32 SideChainService::sideChainTargetCount() const
{
    return 2;
}

bool SideChainService::sideChainTargetEnabled(quint32 index) const
{
    const auto currentSettings = settings(m_trackIndex);
    if (index < currentSettings.targets.size()) {
        return currentSettings.targets.at(index).enabled;
    }
    return false;
}

void SideChainService::setSideChainTargetEnabled(quint32 index, bool enabled)
{
    if (sideChainTargetEnabled(index) != enabled) {
        auto currentSettings = settings(m_trackIndex);
        if (index >= currentSettings.targets.size()) {
            currentSettings.targets.resize(index + 1);
        }
        currentSettings.targets[index].enabled = enabled;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainTargetChanged(index);
    }
}

quint8 SideChainService::sideChainTargetController(quint32 index) const
{
    const auto currentSettings = settings(m_trackIndex);
    if (index < currentSettings.targets.size()) {
        return currentSettings.targets.at(index).controller;
    }
    return 0;
}

void SideChainService::setSideChainTargetController(quint32 index, quint8 controller)
{
    if (sideChainTargetController(index) != controller) {
        auto currentSettings = settings(m_trackIndex);
        if (index >= currentSettings.targets.size()) {
            currentSettings.targets.resize(index + 1);
        }
        currentSettings.targets[index].controller = controller;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainTargetChanged(index);
    }
}

quint8 SideChainService::sideChainTargetTargetValue(quint32 index) const
{
    const auto currentSettings = settings(m_trackIndex);
    if (index < currentSettings.targets.size()) {
        return currentSettings.targets.at(index).targetValue;
    }
    return 0;
}

void SideChainService::setSideChainTargetTargetValue(quint32 index, quint8 value)
{
    if (sideChainTargetTargetValue(index) != value) {
        auto currentSettings = settings(m_trackIndex);
        if (index >= currentSettings.targets.size()) {
            currentSettings.targets.resize(index + 1);
        }
        currentSettings.targets[index].targetValue = value;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainTargetChanged(index);
    }
}

quint8 SideChainService::sideChainTargetReleaseValue(quint32 index) const
{
    const auto currentSettings = settings(m_trackIndex);
    if (index < currentSettings.targets.size()) {
        return currentSettings.targets.at(index).releaseValue;
    }
    return 0;
}

void SideChainService::setSideChainTargetReleaseValue(quint32 index, quint8 value)
{
    if (sideChainTargetReleaseValue(index) != value) {
        auto currentSettings = settings(m_trackIndex);
        if (index >= currentSettings.targets.size()) {
            currentSettings.targets.resize(index + 1);
        }
        currentSettings.targets[index].releaseValue = value;
        setSettings(m_trackIndex, currentSettings);
        emit sideChainTargetChanged(index);
    }
}

} // namespace noteahead
