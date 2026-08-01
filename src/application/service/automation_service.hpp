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

#ifndef AUTOMATION_SERVICE_HPP
#define AUTOMATION_SERVICE_HPP

#include <QObject>

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "../../domain/midi/midi_cc_automation.hpp"
#include "../../domain/midi/pitch_bend_automation.hpp"
#include "../../domain/tracker/event.hpp"

namespace noteahead {

class ProjectReader;
class ProjectWriter;

struct Position;

class PropertyService;

class AutomationService : public QObject
{
    Q_OBJECT

public:
    AutomationService(std::shared_ptr<PropertyService> propertyService);

    //! Resolves the MIDI port a track plays through.
    //!
    //! Value ranges are per port — an internal device may accept more than MIDI 1.0 allows — and
    //! that range is not only a clamp here but the scale modulation depth is measured against, so
    //! rendering has to know the destination. Injected rather than depended on directly to keep
    //! this service out of the song's business.
    using PortNameResolver = std::function<QString(size_t track)>;
    void setPortNameResolver(PortNameResolver resolver);

    void clear();

    void deletePatterns(const std::set<size_t> & patternsToDelete);

    // <-- API for QML/UI -->

    //! Adds automation for MIDI CC events.
    //! \param value0 Start value from 0 to 127.
    //! \param value1 End value from 0 to 127.
    Q_INVOKABLE quint64 addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, bool enabled, quint8 eventsPerBeat, quint8 lineOffset);
    Q_INVOKABLE quint64 addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, quint8 eventsPerBeat, quint8 lineOffset);
    quint64 addMidiCcAutomation(const MidiCcAutomation & automation);
    void addMidiCcAutomationWithId(const MidiCcAutomation & automation);
    Q_INVOKABLE void addMidiCcModulation(quint64 automationId, int type, quint64 cycles, float amplitude, float offset, bool inverted);

    //! Adds automation for MIDI pitch bend.
    //! \param value0 Start value from -100% to +100%.
    //! \param value1 End value from -100% to +100%.
    Q_INVOKABLE quint64 addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment, bool enabled);
    Q_INVOKABLE quint64 addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment);
    quint64 addPitchBendAutomation(const PitchBendAutomation & automation);
    void addPitchBendAutomationWithId(const PitchBendAutomation & automation);
    Q_INVOKABLE void addPitchBendModulation(quint64 automationId, int type, quint64 cycles, float amplitude, float offset, bool inverted);
    Q_INVOKABLE bool hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    Q_INVOKABLE double automationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    //! One automation's value on each line of a range, normalized to 0..1 so MIDI CC and pitch bend
    //! share an axis. Lines the automation does not cover are left unset, which is what makes the
    //! drawn curve stop at its ends instead of running to the edge of the pattern.
    struct AutomationCurve
    {
        quint64 id { 0 };
        bool isPitchBend { false };
        std::vector<std::optional<double>> values;
    };

    using AutomationCurveList = std::vector<AutomationCurve>;
    //! Every automation on a column, sampled per line. One call per repaint rather than one per
    //! line per automation, since sampling walks the automation list and applies its modulation.
    AutomationCurveList automationCurves(quint64 pattern, quint64 track, quint64 column, quint64 startLine, quint64 endLine) const;

    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    MidiCcAutomationList midiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    MidiCcAutomationList midiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const;
    MidiCcAutomationList midiCcAutomationsByTrack(quint64 pattern, quint64 track) const;
    MidiCcAutomationList midiCcAutomationsByPattern(quint64 pattern) const;
    MidiCcAutomationList midiCcAutomations() const;

    using PitchBendAutomationList = std::vector<PitchBendAutomation>;
    PitchBendAutomationList pitchBendAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    PitchBendAutomationList pitchBendAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const;
    PitchBendAutomationList pitchBendAutomationsByTrack(quint64 pattern, quint64 track) const;
    PitchBendAutomationList pitchBendAutomationsByPattern(quint64 pattern) const;
    PitchBendAutomationList pitchBendAutomations() const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine, size_t linesPerBeat) const;

    void deserializeFromXml(ProjectReader & reader);
    void serializeToXml(ProjectWriter & writer) const;

public slots:
    void deleteMidiCcAutomation(const MidiCcAutomation & automationToDelete);
    void updateMidiCcAutomation(const MidiCcAutomation & updatedAutomation);
    void deletePitchBendAutomation(const PitchBendAutomation & automationToDelete);
    void updatePitchBendAutomation(const PitchBendAutomation & updatedAutomation);

signals:
    void lineDataChanged(const Position & position);

private:
    bool hasMidiCcAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    bool hasPitchBendAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    double midiCcAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    double pitchBendAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    //! Interpolation plus modulation for one automation on one line, in the automation's own units.
    //! Shared with the event rendering so what is drawn cannot drift from what is played.
    int midiCcValueAt(const MidiCcAutomation & automation, size_t line) const;
    int pitchBendValueAt(const PitchBendAutomation & automation, size_t line) const;
    void notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1);
    void notifyChangedLines(const MidiCcAutomation & automation);
    void notifyChangedLinesMerged(const MidiCcAutomation & automation1, const MidiCcAutomation & automation2);
    void notifyChangedLines(const PitchBendAutomation & automation);
    void notifyChangedLinesMerged(const PitchBendAutomation & automation1, const PitchBendAutomation & automation2);
    EventList renderMidiCcToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderPitchBendToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderMidiCcToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine, size_t linesPerBeat) const;
    EventList renderPitchBendToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const;

    double sineModulationValue(const ModulationParameters & modulation, double phase) const;
    double randomModulationValue(size_t automationId, const ModulationParameters & modulation, double phase) const;

    struct Automations
    {
        MidiCcAutomationList midiCc;
        PitchBendAutomationList pitchBend;
    };

    //! Value range of a controller at the given track's destination.
    int controllerMaxValue(uint8_t controller, size_t track) const;

    Automations m_automations;
    std::shared_ptr<PropertyService> m_propertyService;
    PortNameResolver m_portNameResolver;
};

} // namespace noteahead

#endif // AUTOMATION_SERVICE_HPP
