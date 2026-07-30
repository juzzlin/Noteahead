// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "auto_note_off_offset.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <cmath>

namespace noteahead {

namespace {

//! Beats in a whole note. The tracker has no time signature, so a whole note is four beats.
const double beatsPerWholeNote = 4.0;

} // namespace

AutoNoteOffOffset::AutoNoteOffOffset(std::chrono::milliseconds milliseconds)
  : m_milliseconds { milliseconds }
{
}

AutoNoteOffOffset::AutoNoteOffOffset(int syncDenominator)
  : m_mode { Mode::Sync }
  , m_syncDenominator { syncDenominator }
{
}

AutoNoteOffOffset::Mode AutoNoteOffOffset::mode() const
{
    return m_mode;
}

void AutoNoteOffOffset::setMode(Mode mode)
{
    m_mode = mode;
}

bool AutoNoteOffOffset::syncEnabled() const
{
    return m_mode == Mode::Sync;
}

void AutoNoteOffOffset::setSyncEnabled(bool enabled)
{
    m_mode = enabled ? Mode::Sync : Mode::Milliseconds;
}

std::chrono::milliseconds AutoNoteOffOffset::milliseconds() const
{
    return m_milliseconds;
}

void AutoNoteOffOffset::setMilliseconds(std::chrono::milliseconds milliseconds)
{
    m_milliseconds = milliseconds;
}

int AutoNoteOffOffset::syncDenominator() const
{
    return m_syncDenominator;
}

void AutoNoteOffOffset::setSyncDenominator(int denominator)
{
    m_syncDenominator = denominator;
}

const std::vector<int> & AutoNoteOffOffset::syncDenominators()
{
    static const std::vector<int> denominators = { 4, 8, 16, 24, 32, 48, 64, 128 };
    return denominators;
}

size_t AutoNoteOffOffset::ticks(size_t beatsPerMinute, size_t linesPerBeat, size_t ticksPerLine) const
{
    if (m_mode == Mode::Sync) {
        if (m_syncDenominator <= 0) {
            return 0;
        }
        const double ticksPerBeat = static_cast<double>(linesPerBeat) * static_cast<double>(ticksPerLine);
        return static_cast<size_t>(std::round(ticksPerBeat * beatsPerWholeNote / static_cast<double>(m_syncDenominator)));
    }

    const double linesPerMinute = static_cast<double>(beatsPerMinute) * static_cast<double>(linesPerBeat);
    const double offsetLines = static_cast<double>(m_milliseconds.count()) * linesPerMinute / 60'000.0;
    return static_cast<size_t>(offsetLines * static_cast<double>(ticksPerLine));
}

void AutoNoteOffOffset::serializeToXmlAttributes(ProjectWriter & writer) const
{
    // Both values go out regardless of the active mode so that toggling the mode in the UI and
    // saving does not lose the setting the other mode had.
    writer.writeAttribute(Constants::NahdXml::xmlKeyAutoNoteOffOffset(), QString::number(m_milliseconds.count()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyAutoNoteOffSyncEnabled(), syncEnabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyAutoNoteOffSyncDenominator(), QString::number(m_syncDenominator));
}

AutoNoteOffOffset AutoNoteOffOffset::deserializeFromXmlAttributes(ProjectReader & reader)
{
    AutoNoteOffOffset offset;

    // A project written before sync mode existed has the milliseconds attribute and nothing else,
    // which lands exactly on the defaults here.
    if (const auto milliseconds = Utils::Xml::readMSecAttribute(reader, Constants::NahdXml::xmlKeyAutoNoteOffOffset(), false); milliseconds.has_value()) {
        offset.setMilliseconds(*milliseconds);
    }
    if (const auto denominator = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyAutoNoteOffSyncDenominator(), false); denominator.has_value()) {
        offset.setSyncDenominator(*denominator);
    }
    offset.setSyncEnabled(Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyAutoNoteOffSyncEnabled(), false).value_or(false));

    return offset;
}

bool AutoNoteOffOffset::operator==(const AutoNoteOffOffset & other) const
{
    return m_mode == other.m_mode && m_milliseconds == other.m_milliseconds && m_syncDenominator == other.m_syncDenominator;
}

bool AutoNoteOffOffset::operator!=(const AutoNoteOffOffset & other) const
{
    return !(*this == other);
}

QString AutoNoteOffOffset::toString() const
{
    return syncEnabled() ? QString { "1/%1" }.arg(m_syncDenominator) : QString { "%1 ms" }.arg(m_milliseconds.count());
}

} // namespace noteahead
