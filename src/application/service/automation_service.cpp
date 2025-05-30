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
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/pitch_bend_data.hpp"
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

quint64 AutomationService::addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, bool enabled)
{
    const auto maxIdItem = std::max_element(m_automations.midiCc.begin(), m_automations.midiCc.end(), [](auto && lhs, auto && rhs) { return lhs.id() < rhs.id(); });
    const auto id = maxIdItem != m_automations.midiCc.end() ? (*maxIdItem).id() + 1 : 1;
    const AutomationLocation location = { pattern, track, column };
    const MidiCcAutomation::InterpolationParameters parameters = { line0, line1, value0, value1 };
    const auto automation = MidiCcAutomation { id, location, controller, parameters, comment, enabled };
    m_automations.midiCc.push_back(automation);
    notifyChangedLines(pattern, track, column, line0, line1);
    juzzlin::L(TAG).info() << "MIDI CC Automation added: " << automation.toString().toStdString();
    return automation.id();
}

quint64 AutomationService::addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment)
{
    return addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, true);
}

void AutomationService::deleteMidiCcAutomation(const MidiCcAutomation & automationToDelete)
{
    if (const auto iter = std::ranges::find_if(m_automations.midiCc, [&](auto && existingAutomation) {
            return automationToDelete.id() == existingAutomation.id();
        });
        iter != m_automations.midiCc.end()) {
        m_automations.midiCc.erase(iter);
        notifyChangedLines(automationToDelete);
        juzzlin::L(TAG).info() << "MIDI CC Automation deleted: " << automationToDelete.toString().toStdString();
    } else {
        juzzlin::L(TAG).error() << "No such automation id to delete: " << automationToDelete.id();
    }
}

void AutomationService::updateMidiCcAutomation(const MidiCcAutomation & updatedAutomation)
{
    if (const auto iter = std::ranges::find_if(m_automations.midiCc, [&](auto && existingAutomation) {
            return updatedAutomation.id() == existingAutomation.id();
        });
        iter != m_automations.midiCc.end()) {
        if (const auto oldAutomation = *iter; oldAutomation != updatedAutomation) {
            *iter = updatedAutomation;
            if (oldAutomation.interpolation() != updatedAutomation.interpolation() || //
                oldAutomation.enabled() != updatedAutomation.enabled()) {
                notifyChangedLinesMerged(oldAutomation, updatedAutomation);
            }
            juzzlin::L(TAG).info() << "MIDI CC Automation updated: " << updatedAutomation.toString().toStdString();
        } else {
            juzzlin::L(TAG).info() << "No changes for MIDI CC Automation: " << updatedAutomation.toString().toStdString();
        }
    } else {
        juzzlin::L(TAG).error() << "No such automation id: " << updatedAutomation.id();
    }
}

quint64 AutomationService::addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment)
{
    return addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment, true);
}

quint64 AutomationService::addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment, bool enabled)
{
    const auto maxIdItem = std::max_element(m_automations.pitchBend.begin(), m_automations.pitchBend.end(), [](auto && lhs, auto && rhs) { return lhs.id() < rhs.id(); });
    const auto id = maxIdItem != m_automations.pitchBend.end() ? (*maxIdItem).id() + 1 : 1;
    const AutomationLocation location = { pattern, track, column };
    const PitchBendAutomation::InterpolationParameters parameters = { line0, line1, value0, value1 };
    const auto automation = PitchBendAutomation { id, location, parameters, comment, enabled };
    m_automations.pitchBend.push_back(automation);
    notifyChangedLines(pattern, track, column, line0, line1);
    juzzlin::L(TAG).info() << "Pitch Bend Automation added: " << automation.toString().toStdString();
    return automation.id();
}

void AutomationService::deletePitchBendAutomation(const PitchBendAutomation & automationToDelete)
{
    if (const auto iter = std::ranges::find_if(m_automations.pitchBend, [&](auto && existingAutomation) {
            return automationToDelete.id() == existingAutomation.id();
        });
        iter != m_automations.pitchBend.end()) {
        m_automations.pitchBend.erase(iter);
        notifyChangedLines(automationToDelete);
        juzzlin::L(TAG).info() << "Pitch Bend Automation deleted: " << automationToDelete.toString().toStdString();
    } else {
        juzzlin::L(TAG).error() << "No such automation id to delete: " << automationToDelete.id();
    }
}

void AutomationService::updatePitchBendAutomation(const PitchBendAutomation & updatedAutomation)
{
    if (const auto iter = std::ranges::find_if(m_automations.pitchBend, [&](auto && existingAutomation) {
            return updatedAutomation.id() == existingAutomation.id();
        });
        iter != m_automations.pitchBend.end()) {
        if (const auto oldAutomation = *iter; oldAutomation != updatedAutomation) {
            *iter = updatedAutomation;
            if (oldAutomation.interpolation() != updatedAutomation.interpolation() || //
                oldAutomation.enabled() != updatedAutomation.enabled()) {
                notifyChangedLinesMerged(oldAutomation, updatedAutomation);
            }
            juzzlin::L(TAG).info() << "Pitch Bend Automation updated: " << updatedAutomation.toString().toStdString();
        } else {
            juzzlin::L(TAG).info() << "No changes for Pitch Bend Automation: " << updatedAutomation.toString().toStdString();
        }
    } else {
        juzzlin::L(TAG).error() << "No such automation id: " << updatedAutomation.id();
    }
}

bool AutomationService::hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    return hasMidiCcAutomations(pattern, track, column, line) || hasPitchBendAutomations(pattern, track, column, line);
}

bool AutomationService::hasMidiCcAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    const auto match = std::ranges::find_if(m_automations.midiCc, [&](auto && automation) {
        auto && location = automation.location();
        auto && interpolation = automation.interpolation();
        return location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1;
    });
    return match != m_automations.midiCc.end();
}

bool AutomationService::hasPitchBendAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    const auto match = std::ranges::find_if(m_automations.pitchBend, [&](auto && automation) {
        auto && location = automation.location();
        auto && interpolation = automation.interpolation();
        return location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1;
    });
    return match != m_automations.pitchBend.end();
}

double AutomationService::automationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    return std::max(midiCcAutomationWeight(pattern, track, column, line), pitchBendAutomationWeight(pattern, track, column, line));
}

double AutomationService::midiCcAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    if (const auto events = renderMidiCcToEventsByLine(pattern, track, column, line, 0); !events.empty()) {
        const double sum = std::accumulate(events.begin(), events.end(), 0.0, [](double acc, auto && event) {
            return acc + (event->midiCcData() ? event->midiCcData()->normalizedValue() : 0);
        });
        return sum / static_cast<int>(events.size());
    } else {
        return 0;
    }
}

double AutomationService::pitchBendAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    if (const auto events = renderPitchBendToEventsByLine(pattern, track, column, line, 0); !events.empty()) {
        const double sum = std::accumulate(events.begin(), events.end(), 0.0, [](double acc, auto && event) {
            return acc + (event->pitchBendData() ? event->pitchBendData()->normalizedValue() : 0);
        });
        return sum / static_cast<int>(events.size());
    } else {
        return 0;
    }
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    MidiCcAutomationList automations;
    std::ranges::copy(m_automations.midiCc
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              auto && interpolation = automation.interpolation();
                              return location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const
{
    MidiCcAutomationList automations;
    std::ranges::copy(m_automations.midiCc
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern && location.track() == track && location.column() == column;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByTrack(quint64 pattern, quint64 track) const
{
    MidiCcAutomationList automations;
    std::ranges::copy(m_automations.midiCc
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern && location.track() == track;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomationsByPattern(quint64 pattern) const
{
    MidiCcAutomationList automations;
    std::ranges::copy(m_automations.midiCc
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::MidiCcAutomationList AutomationService::midiCcAutomations() const
{
    return m_automations.midiCc;
}

AutomationService::PitchBendAutomationList AutomationService::pitchBendAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    PitchBendAutomationList automations;
    std::ranges::copy(m_automations.pitchBend
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              auto && interpolation = automation.interpolation();
                              return location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::PitchBendAutomationList AutomationService::pitchBendAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const
{
    PitchBendAutomationList automations;
    std::ranges::copy(m_automations.pitchBend
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern && location.track() == track && location.column() == column;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::PitchBendAutomationList AutomationService::pitchBendAutomationsByTrack(quint64 pattern, quint64 track) const
{
    PitchBendAutomationList automations;
    std::ranges::copy(m_automations.pitchBend
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern && location.track() == track;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::PitchBendAutomationList AutomationService::pitchBendAutomationsByPattern(quint64 pattern) const
{
    PitchBendAutomationList automations;
    std::ranges::copy(m_automations.pitchBend
                        | std::views::filter([&](auto && automation) {
                              auto && location = automation.location();
                              return location.pattern() == pattern;
                          }),
                      std::back_inserter(automations));
    return automations;
}

AutomationService::PitchBendAutomationList AutomationService::pitchBendAutomations() const
{
    return m_automations.pitchBend;
}

AutomationService::EventList AutomationService::renderToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const
{
    EventList events;
    std::ranges::copy(renderMidiCcToEventsByLine(pattern, track, column, line, tick), std::back_inserter(events));
    std::ranges::copy(renderPitchBendToEventsByLine(pattern, track, column, line, tick), std::back_inserter(events));
    return events;
}

AutomationService::EventList AutomationService::renderMidiCcToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const
{
    EventList events;

    for (const auto & automation : m_automations.midiCc) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1) {
                Interpolator interpolator {
                    static_cast<size_t>(interpolation.line0),
                    static_cast<size_t>(interpolation.line1),
                    static_cast<double>(interpolation.value0),
                    static_cast<double>(interpolation.value1)
                };
                const auto clampedValue = std::clamp(static_cast<int>(interpolator.getValue(static_cast<size_t>(line))), 0, 127); // MIDI CC value range
                events.push_back(std::make_shared<Event>(tick, MidiCcData { track, column, automation.controller(), static_cast<uint8_t>(clampedValue) }));
            }
        }
    }

    return events;
}

AutomationService::EventList AutomationService::renderPitchBendToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const
{
    EventList events;

    for (const auto & automation : m_automations.pitchBend) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern() == pattern && location.track() == track && location.column() == column && line >= interpolation.line0 && line <= interpolation.line1) {
                Interpolator interpolator {
                    static_cast<size_t>(interpolation.line0),
                    static_cast<size_t>(interpolation.line1),
                    static_cast<double>(interpolation.value0),
                    static_cast<double>(interpolation.value1)
                };
                const auto percentage = std::clamp(static_cast<int>(interpolator.getValue(static_cast<size_t>(line))), -100, 100);
                events.push_back(std::make_shared<Event>(tick, PitchBendData { track, column, static_cast<double>(percentage) }));
            }
        }
    }

    return events;
}

AutomationService::EventList AutomationService::renderToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const
{
    EventList events;
    std::ranges::copy(renderMidiCcToEventsByColumn(pattern, track, column, tick, ticksPerLine), std::back_inserter(events));
    std::ranges::copy(renderPitchBendToEventsByColumn(pattern, track, column, tick, ticksPerLine), std::back_inserter(events));
    return events;
}

AutomationService::EventList AutomationService::renderMidiCcToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const
{
    EventList events;

    for (const auto & automation : m_automations.midiCc) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern() == pattern && location.track() == track && location.column() == column) {
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

AutomationService::EventList AutomationService::renderPitchBendToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const
{
    EventList events;

    for (const auto & automation : m_automations.pitchBend) {
        if (automation.enabled()) {
            const auto & location = automation.location();
            const auto & interpolation = automation.interpolation();
            if (location.pattern() == pattern && location.track() == track && location.column() == column) {
                Interpolator interpolator {
                    static_cast<size_t>(interpolation.line0),
                    static_cast<size_t>(interpolation.line1),
                    static_cast<double>(interpolation.value0),
                    static_cast<double>(interpolation.value1)
                };
                std::optional<double> prevValue;
                for (size_t line = interpolation.line0; line <= interpolation.line1; line++) {
                    const auto percentage = std::clamp(static_cast<int>(interpolator.getValue(static_cast<size_t>(line))), -100, 100);
                    const double minDiff = 200.0 / 16383;
                    if (!prevValue || std::fabs(*prevValue - percentage) > minDiff) {
                        events.push_back(std::make_shared<Event>(tick + line * ticksPerLine, PitchBendData { track, column, static_cast<double>(percentage) }));
                        prevValue = percentage;
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

    m_automations = {};
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

void AutomationService::notifyChangedLines(const MidiCcAutomation & automation)
{
    const auto location = automation.location();
    const auto interpolation = automation.interpolation();
    notifyChangedLines(location.pattern(), location.track(), location.column(), interpolation.line0, interpolation.line1);
}

void AutomationService::notifyChangedLinesMerged(const MidiCcAutomation & automation1, const MidiCcAutomation & automation2)
{
    const auto location = automation1.location();
    const auto interpolation1 = automation1.interpolation();
    const auto interpolation2 = automation2.interpolation();
    notifyChangedLines(location.pattern(), location.track(), location.column(), std::min(interpolation1.line0, interpolation2.line0), std::max(interpolation1.line1, interpolation2.line1));
}

void AutomationService::notifyChangedLines(const PitchBendAutomation & automation)
{
    const auto location = automation.location();
    const auto interpolation = automation.interpolation();
    notifyChangedLines(location.pattern(), location.track(), location.column(), interpolation.line0, interpolation.line1);
}

void AutomationService::notifyChangedLinesMerged(const PitchBendAutomation & automation1, const PitchBendAutomation & automation2)
{
    const auto location = automation1.location();
    const auto interpolation1 = automation1.interpolation();
    const auto interpolation2 = automation2.interpolation();
    notifyChangedLines(location.pattern(), location.track(), location.column(), std::min(interpolation1.line0, interpolation2.line0), std::max(interpolation1.line1, interpolation2.line1));
}

void AutomationService::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).info() << "Deserializing";
    m_automations = {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyAutomation()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMidiCcAutomation())) {
            if (const auto automation = MidiCcAutomation::deserializeFromXml(reader); automation) {
                automation->setId(m_automations.midiCc.size() + 1); // Assign id on-the-fly
                m_automations.midiCc.push_back(*automation);
            }
        } else if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPitchBendAutomation())) {
            if (const auto automation = PitchBendAutomation::deserializeFromXml(reader); automation) {
                automation->setId(m_automations.pitchBend.size() + 1); // Assign id on-the-fly
                m_automations.pitchBend.push_back(*automation);
            }
        }
        reader.readNext();
    }
}

void AutomationService::serializeToXml(QXmlStreamWriter & writer) const
{
    juzzlin::L(TAG).info() << "Serializing";

    writer.writeStartElement(Constants::NahdXml::xmlKeyAutomation());

    for (const auto & automation : m_automations.midiCc) {
        automation.serializeToXml(writer);
    }

    for (const auto & automation : m_automations.pitchBend) {
        automation.serializeToXml(writer);
    }

    writer.writeEndElement(); // AutomationService
}

} // namespace noteahead
