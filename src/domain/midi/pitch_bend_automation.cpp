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

#include "pitch_bend_automation.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <QVariant>

namespace noteahead {

PitchBendAutomation::PitchBendAutomation() = default;

PitchBendAutomation::PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, ModulationParameters modulation, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_interpolation { interpolation }
  , m_modulation { modulation }
{
}

PitchBendAutomation::PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment, bool enabled)
  : PitchBendAutomation { id, location, interpolation, {}, comment, enabled }
{
}

PitchBendAutomation::PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment)
  : PitchBendAutomation { id, location, interpolation, {}, comment, true }
{
}

bool PitchBendAutomation::operator==(const PitchBendAutomation & other) const
{
    return id() == other.id() && //
      location() == other.location() && //
      m_interpolation == other.m_interpolation && //
      m_modulation == other.m_modulation && //
      comment() == other.comment() && //
      enabled() == other.enabled();
}

bool PitchBendAutomation::operator!=(const PitchBendAutomation & other) const
{
    return !(*this == other);
}

bool PitchBendAutomation::operator<(const PitchBendAutomation & other) const
{
    return id() < other.id();
}

const PitchBendAutomation::InterpolationParameters & PitchBendAutomation::interpolation() const
{
    return m_interpolation;
}

void PitchBendAutomation::setInterpolation(const InterpolationParameters & interpolation)
{
    m_interpolation = interpolation;
}

const ModulationParameters & PitchBendAutomation::modulation() const
{
    return m_modulation;
}

void PitchBendAutomation::setModulation(const ModulationParameters & modulation)
{
    m_modulation = modulation;
}

QString PitchBendAutomation::toString() const
{
    return QString { "PitchBendAutomation(id=%1, pattern=%2, track=%3, column=%4, "
                     "line: %5 -> %6, value: %7 -> %8), enabled=%9" }
      .arg(QString::number(id()),
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           QString::number(enabled()));
}

void PitchBendAutomation::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPitchBendAutomation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyComment(), comment());
    writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), enabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    location().serializeToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInterpolation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine0(), QString::number(m_interpolation.line0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine1(), QString::number(m_interpolation.line1));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue0(), QString::number(m_interpolation.value0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue1(), QString::number(m_interpolation.value1));
    writer.writeEndElement(); // Interpolation

    if (m_modulation.cycles > 0.f || m_modulation.amplitude > 0.f || m_modulation.offset != 0.f) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyModulation());
        writer.writeAttribute(Constants::NahdXml::xmlKeyType(), m_modulation.type == ModulationParameters::ModulationType::SineWave ? Constants::NahdXml::xmlValueSineWave() : Constants::NahdXml::xmlValueRandom());
        writer.writeAttribute(Constants::NahdXml::xmlKeyCycles(), QString::number(static_cast<int>(m_modulation.cycles)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyAmplitude(), QString::number(static_cast<int>(m_modulation.amplitude)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyOffset(), QString::number(static_cast<int>(m_modulation.offset)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyInverted(), m_modulation.inverted ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
        writer.writeEndElement(); // Modulation
    }

    writer.writeEndElement(); // Automation
}

PitchBendAutomation::PitchBendAutomationU PitchBendAutomation::deserializeFromXml(ProjectReader & reader)
{
    const QString comment = reader.attribute(Constants::NahdXml::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled(), false).value_or(true);
    AutomationLocation::AutomationLocationU location = std::make_unique<AutomationLocation>();
    PitchBendAutomation::InterpolationParameters parameters {};
    ModulationParameters modulationParameters {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPitchBendAutomation()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyLocation())) {
                location = AutomationLocation::deserializeFromXml(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyInterpolation())) {
                parameters.line0 = reader.attribute(Constants::NahdXml::xmlKeyLine0()).toULongLong();
                parameters.line1 = reader.attribute(Constants::NahdXml::xmlKeyLine1()).toULongLong();
                parameters.value0 = reader.attribute(Constants::NahdXml::xmlKeyValue0()).toInt();
                parameters.value1 = reader.attribute(Constants::NahdXml::xmlKeyValue1()).toInt();
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyModulation())) {
                if (const auto type = reader.attribute(Constants::NahdXml::xmlKeyType()); type.isValid()) {
                    if (type.toString() == Constants::NahdXml::xmlValueRandom()) {
                        modulationParameters.type = ModulationParameters::ModulationType::Random;
                    } else {
                        modulationParameters.type = ModulationParameters::ModulationType::SineWave;
                    }
                } else {
                    modulationParameters.type = ModulationParameters::ModulationType::SineWave;
                }

                if (const auto cycles = reader.attribute(Constants::NahdXml::xmlKeyCycles()); cycles.isValid()) {
                    modulationParameters.cycles = static_cast<float>(cycles.toInt());
                } else if (const auto sineCycles = reader.attribute("sineCycles"); sineCycles.isValid()) {
                    modulationParameters.cycles = static_cast<float>(sineCycles.toInt());
                }

                if (const auto amplitude = reader.attribute(Constants::NahdXml::xmlKeyAmplitude()); amplitude.isValid()) {
                    modulationParameters.amplitude = static_cast<float>(amplitude.toInt());
                } else if (const auto sineAmplitude = reader.attribute("sineAmplitude"); sineAmplitude.isValid()) {
                    modulationParameters.amplitude = static_cast<float>(sineAmplitude.toInt());
                }

                if (const auto offset = reader.attribute(Constants::NahdXml::xmlKeyOffset()); offset.isValid()) {
                    modulationParameters.offset = static_cast<float>(offset.toInt());
                } else if (const auto sineOffset = reader.attribute("sineOffset"); sineOffset.isValid()) {
                    modulationParameters.offset = static_cast<float>(sineOffset.toInt());
                }

                const auto inverted = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyInverted(), false);
                if (inverted.has_value()) {
                    modulationParameters.inverted = inverted.value();
                } else {
                    modulationParameters.inverted = Utils::Xml::readBoolAttribute(reader, "sineInverted", false).value_or(false);
                }
            }
        }
    }
    return std::make_unique<PitchBendAutomation>(0, *location, parameters, modulationParameters, comment, enabled);
}

} // namespace noteahead
