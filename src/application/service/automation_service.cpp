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

#include "automation_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/interpolator.hpp"
#include "../position.hpp"

#include <algorithm>
#include <ranges>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "AutomationService";

AutomationService::AutomationService()
{
}

void AutomationService::addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, bool enabled)
{
    const auto maxIdItem = std::max_element(m_midiCcAutomations.begin(), m_midiCcAutomations.end(), [](auto && lhs, auto && rhs) { return lhs.id() > rhs.id(); });
    const auto id = maxIdItem != m_midiCcAutomations.end() ? (*maxIdItem).id() : 1;
    MidiCcAutomation::Location location = { pattern, track, column };
    MidiCcAutomation::Interpolation interpolation = { line0, line1, value0, value1 };
    const auto automation = MidiCcAutomation { id, location, controller, interpolation, comment, enabled };
    m_midiCcAutomations.push_back(automation);
    notifyChangedLines(pattern, track, column, line0, line1);
    juzzlin::L(TAG).info() << "MIDI CC Automation added: " << automation.toString().toStdString();
}

void AutomationService::addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment)
{
    addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, true);
}

void AutomationService::deleteMidiCcAutomation(const MidiCcAutomation & midiCcAutomationToDelete)
{
    if (const auto iter = std::ranges::find_if(m_midiCcAutomations, [&](auto && existingMidiCcAutomation) {
            return midiCcAutomationToDelete.id() == existingMidiCcAutomation.id();
        });
        iter != m_midiCcAutomations.end()) {
        m_midiCcAutomations.erase(iter);
        notifyChangedLines(midiCcAutomationToDelete);
        juzzlin::L(TAG).info() << "MIDI CC Automation deleted: " << midiCcAutomationToDelete.toString().toStdString();
    } else {
        juzzlin::L(TAG).error() << "No such automation id to delete: " << midiCcAutomationToDelete.id();
    }
}

void AutomationService::updateMidiCcAutomation(const MidiCcAutomation & updatedMidiCcAutomation)
{
    if (const auto iter = std::ranges::find_if(m_midiCcAutomations, [&](auto && existingMidiCcAutomation) {
            return updatedMidiCcAutomation.id() == existingMidiCcAutomation.id();
        });
        iter != m_midiCcAutomations.end()) {
        if (const auto oldAutomation = *iter; oldAutomation != updatedMidiCcAutomation) {
            *iter = updatedMidiCcAutomation;
            if (oldAutomation.interpolation() != updatedMidiCcAutomation.interpolation() || //
                oldAutomation.enabled() != updatedMidiCcAutomation.enabled()) {
                notifyChangedLinesMerged(oldAutomation, updatedMidiCcAutomation);
            }
            juzzlin::L(TAG).info() << "MIDI CC Automation updated: " << updatedMidiCcAutomation.toString().toStdString();
        } else {
            juzzlin::L(TAG).info() << "No changes for MIDI CC Automation: " << updatedMidiCcAutomation.toString().toStdString();
        }
    } else {
        juzzlin::L(TAG).error() << "No such automation id: " << updatedMidiCcAutomation.id();
    }
}

bool AutomationService::hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    const auto match = std::ranges::find_if(m_midiCcAutomations, [&](auto && automation) {
        auto && location = automation.location();
        auto && interpolation = automation.interpolation();
        return location.pattern == pattern && location.track == track && location.column == column && line >= interpolation.line0 && line <= interpolation.line1;
    });
    return match != m_midiCcAutomations.end();
}

double AutomationService::automationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    if (const auto events = renderToEventsByLine(pattern, track, column, line, 0); !events.empty()) {
        const double sum = std::accumulate(events.begin(), events.end(), 0, [](double acc, auto && event) {
            return acc + (event->midiCcData() ? event->midiCcData()->value() : 0);
        });
        return sum / (static_cast<int>(events.size()) * 127);
    } else {
        return 0;
    }
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    MidiCcAutomationList midiCcAutomations;
    std::ranges::copy(m_midiCcAutomations
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              auto && interpolation = automation.interpolation();
                              return location.pattern == pattern && location.track == track && location.column == column && line >= interpolation.line0 && line <= interpolation.line1;
                          }),
                      std::back_inserter(midiCcAutomations));
    return midiCcAutomations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const
{
    MidiCcAutomationList midiCcAutomations;
    std::ranges::copy(m_midiCcAutomations
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern == pattern && location.track == track && location.column == column;
                          }),
                      std::back_inserter(midiCcAutomations));
    return midiCcAutomations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByTrack(quint64 pattern, quint64 track) const
{
    MidiCcAutomationList midiCcAutomations;
    std::ranges::copy(m_midiCcAutomations
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern == pattern && location.track == track;
                          }),
                      std::back_inserter(midiCcAutomations));
    return midiCcAutomations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByPattern(quint64 pattern) const
{
    MidiCcAutomationList midiCcAutomations;
    std::ranges::copy(m_midiCcAutomations
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern == pattern;
                          }),
                      std::back_inserter(midiCcAutomations));
    return midiCcAutomations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomations() const
{
    return m_midiCcAutomations;
}

AutomationService::EventList AutomationService::renderToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const
{
    EventList events;
    for (const auto & automation : m_midiCcAutomations) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern == pattern && location.track == track && location.column == column && line >= interpolation.line0 && line <= interpolation.line1) {
                Interpolator interpolator {
                    static_cast<size_t>(interpolation.line0),
                    static_cast<size_t>(interpolation.line1),
                    static_cast<double>(interpolation.value0),
                    static_cast<double>(interpolation.value1)
                };
                const auto clampedValue = std::clamp(static_cast<int>(interpolator.getValue(static_cast<size_t>(line))), 0, 127); // MIDI CC value range
                const auto event = std::make_shared<Event>(tick, MidiCcData { track, column, automation.controller(), static_cast<uint8_t>(clampedValue) });
                events.push_back(event);
            }
        }
    }
    return events;
}

AutomationService::EventList AutomationService::renderToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const
{
    EventList events;
    for (const auto & automation : m_midiCcAutomations) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern == pattern && location.track == track && location.column == column) {
                Interpolator interpolator {
                    static_cast<size_t>(interpolation.line0),
                    static_cast<size_t>(interpolation.line1),
                    static_cast<double>(interpolation.value0),
                    static_cast<double>(interpolation.value1)
                };
                std::optional<uint8_t> prevValue;
                for (size_t line = interpolation.line0; line <= interpolation.line1; line++) {
                    const auto clampedValue = std::clamp(static_cast<int>(interpolator.getValue(static_cast<size_t>(line))), 0, 127); // MIDI CC value range
                    if (!prevValue || *prevValue != clampedValue) {
                        events.push_back(std::make_shared<Event>(tick + line * ticksPerLine, MidiCcData { track, column, automation.controller(), static_cast<uint8_t>(clampedValue) }));
                        prevValue = clampedValue;
                    }
                }
            }
        }
    }
    return events;
}

void AutomationService::clear()
{
    juzzlin::L(TAG).info() << "Clearing";

    m_midiCcAutomations.clear();
}

void AutomationService::notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1)
{
    for (auto line = line0; line <= line1; line++) {
        Position changedPosition;
        changedPosition.pattern = pattern;
        changedPosition.track = track;
        changedPosition.column = column;
        changedPosition.line = line;
        emit lineDataChanged(changedPosition);
    }
}

void AutomationService::notifyChangedLines(const MidiCcAutomation & midiCcAutomation)
{
    const auto location = midiCcAutomation.location();
    const auto interpolation = midiCcAutomation.interpolation();
    notifyChangedLines(location.pattern, location.track, location.column, interpolation.line0, interpolation.line1);
}

void AutomationService::notifyChangedLinesMerged(const MidiCcAutomation & midiCcAutomation1, const MidiCcAutomation & midiCcAutomation2)
{
    const auto location = midiCcAutomation1.location();
    const auto interpolation1 = midiCcAutomation1.interpolation();
    const auto interpolation2 = midiCcAutomation2.interpolation();
    notifyChangedLines(location.pattern, location.track, location.column, std::min(interpolation1.line0, interpolation2.line0), std::max(interpolation1.line1, interpolation2.line1));
}

void AutomationService::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).info() << "Deserializing";

    m_midiCcAutomations.clear();
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyAutomation()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyMidiCcAutomation())) {
            if (const auto midiCcAutomation = MidiCcAutomation::deserializeFromXml(reader); midiCcAutomation) {
                midiCcAutomation->setId(m_midiCcAutomations.size() + 1); // Assign id on-the-fly
                m_midiCcAutomations.push_back(*midiCcAutomation);
            }
        }
        reader.readNext();
    }
}

void AutomationService::serializeToXml(QXmlStreamWriter & writer) const
{
    juzzlin::L(TAG).info() << "Serializing";

    writer.writeStartElement(Constants::xmlKeyAutomation());

    for (const auto & automation : m_midiCcAutomations) {
        automation.serializeToXml(writer);
    }

    writer.writeEndElement(); // AutomationService
}

} // namespace noteahead
