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

#include "line_event.hpp"

#include "../common/constants.hpp"
#include "instrument_settings.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

LineEvent::LineEvent(size_t trackIndex, size_t columnIndex)
  : m_trackIndex { trackIndex }
  , m_columnIndex { columnIndex }
{
}

LineEvent::InstrumentSettingsS LineEvent::instrumentSettings() const
{
    return m_instrumentSettings;
}

void LineEvent::setInstrumentSettings(InstrumentSettingsS instrumentSettings)
{
    m_instrumentSettings = instrumentSettings;
    if (m_instrumentSettings) {
        m_instrumentSettings->setTrack(m_trackIndex);
    }
}

bool LineEvent::hasData() const
{
    return m_instrumentSettings != nullptr;
}

void LineEvent::serializeToXml(QXmlStreamWriter & writer) const
{
    if (hasData()) {
        writer.writeStartElement(Constants::xmlKeyLineEvent());
        if (m_instrumentSettings) {
            m_instrumentSettings->serializeToXml(writer);
        }
        writer.writeEndElement(); // LineEvent
    }
}

} // namespace noteahead
