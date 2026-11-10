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

#include "midi_mapping.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <algorithm>

namespace noteahead {

namespace {
//! The kind as saved. A missing attribute is a CC, so the common case writes nothing.
QString kindName(MidiMapping::Kind kind)
{
    switch (kind) {
    case MidiMapping::Kind::Cc:
        break;
    }
    return "cc";
}
} // namespace

double MidiMapping::targetValue(double normalized) const
{
    return std::clamp(min + std::clamp(normalized, 0.0, 1.0) * (max - min), 0.0, 1.0);
}

void MidiMapping::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMidiMapping());
    if (kind != Kind::Cc) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyKind(), kindName(kind));
    }
    writer.writeAttribute(Constants::NahdXml::xmlKeyController(), QString::number(controller));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTarget(), QString::fromStdString(target));
    writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), QString::number(min));
    writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), QString::number(max));
    writer.writeEndElement();
}

std::optional<MidiMapping> MidiMapping::deserializeFromXml(ProjectReader & reader)
{
    MidiMapping mapping;
    const auto kind = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyKind(), false);
    const auto controller = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyController(), false);
    const auto target = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyTarget(), false);
    mapping.min = std::clamp(Utils::Xml::readDoubleAttribute(reader, Constants::NahdXml::xmlKeyMin(), false).value_or(0.0), 0.0, 1.0);
    mapping.max = std::clamp(Utils::Xml::readDoubleAttribute(reader, Constants::NahdXml::xmlKeyMax(), false).value_or(1.0), 0.0, 1.0);
    reader.skipCurrentElement();
    if ((kind && *kind != kindName(Kind::Cc)) || !controller || !target || target->isEmpty()) {
        return std::nullopt;
    }
    mapping.controller = static_cast<uint16_t>(std::min<size_t>(*controller, 0xffff));
    mapping.target = target->toStdString();
    return mapping;
}

} // namespace noteahead
