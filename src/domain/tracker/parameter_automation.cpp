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

#include "parameter_automation.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <QVariant>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

//! Positions are fractions of a knob's travel, so they are written with enough digits to come back
//! as the same double rather than rounded onto the coarse grid an integer attribute would impose.
QString positionToXml(double position)
{
    return QString::number(position, 'g', 12);
}

} // namespace

ParameterAutomation::ParameterAutomation() = default;

ParameterAutomation::ParameterAutomation(size_t id, AutomationLocation location, Target target, Mode mode, InterpolationParameters interpolation, ModulationParameters modulation, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_target { std::move(target) }
  , m_mode { mode }
  , m_interpolation { interpolation }
  , m_modulation { modulation }
{
}

bool ParameterAutomation::operator==(const ParameterAutomation & other) const
{
    return id() == other.id() && //
      m_target == other.m_target && //
      m_mode == other.m_mode && //
      m_eventsPerBeat == other.m_eventsPerBeat && //
      m_lineOffset == other.m_lineOffset && //
      location() == other.location() && //
      m_interpolation == other.m_interpolation && //
      m_modulation == other.m_modulation && //
      comment() == other.comment() && //
      enabled() == other.enabled();
}

bool ParameterAutomation::operator!=(const ParameterAutomation & other) const
{
    return !(*this == other);
}

bool ParameterAutomation::operator<(const ParameterAutomation & other) const
{
    return id() < other.id();
}

const ParameterAutomation::Target & ParameterAutomation::target() const
{
    return m_target;
}

void ParameterAutomation::setTarget(const Target & target)
{
    m_target = target;
}

ParameterAutomation::Mode ParameterAutomation::mode() const
{
    return m_mode;
}

void ParameterAutomation::setMode(Mode mode)
{
    m_mode = mode;
}

const ParameterAutomation::InterpolationParameters & ParameterAutomation::interpolation() const
{
    return m_interpolation;
}

void ParameterAutomation::setInterpolation(const InterpolationParameters & interpolation)
{
    m_interpolation = interpolation;
}

const ModulationParameters & ParameterAutomation::modulation() const
{
    return m_modulation;
}

void ParameterAutomation::setModulation(const ModulationParameters & modulation)
{
    m_modulation = modulation;
}

uint8_t ParameterAutomation::eventsPerBeat() const
{
    return m_eventsPerBeat;
}

void ParameterAutomation::setEventsPerBeat(uint8_t eventsPerBeat)
{
    m_eventsPerBeat = eventsPerBeat;
}

uint8_t ParameterAutomation::lineOffset() const
{
    return m_lineOffset;
}

void ParameterAutomation::setLineOffset(uint8_t lineOffset)
{
    m_lineOffset = lineOffset;
}

QString ParameterAutomation::toString() const
{
    return QString { "ParameterAutomation(id=%1, owner=%2, parameter=%3, mode=%4, pattern=%5, track=%6, column=%7, "
                     "line: %8 -> %9, value: %10 -> %11, curve=%12, enabled=%13)" }
      .arg(QString::number(id()),
           QString::fromStdString(m_target.ownerId),
           QString::fromStdString(m_target.parameter),
           m_mode == Mode::Relative ? C::xmlValueRelative() : QString { "absolute" },
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           Interpolator::curveToXmlValue(m_interpolation.curve),
           QString::number(enabled()));
}

void ParameterAutomation::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(C::xmlKeyParameterAutomation());
    // Absolute is the plain case, so only relative says what it is
    if (m_mode == Mode::Relative) {
        writer.writeAttribute(C::xmlKeyMode(), C::xmlValueRelative());
    }
    writer.writeAttribute(C::xmlKeyEventsPerBeat(), QString::number(m_eventsPerBeat));
    writer.writeAttribute(C::xmlKeyLineOffset(), QString::number(m_lineOffset));
    writer.writeAttribute(C::xmlKeyComment(), comment());
    writer.writeAttribute(C::xmlKeyEnabled(), enabled() ? C::xmlValueTrue() : C::xmlValueFalse());

    writer.writeStartElement(C::xmlKeyAutomationTarget());
    writer.writeAttribute(C::xmlKeyOwner(), C::xmlValueEffect());
    writer.writeAttribute(C::xmlKeyOwnerId(), QString::fromStdString(m_target.ownerId));
    writer.writeAttribute(C::xmlKeyParameterName(), QString::fromStdString(m_target.parameter));
    writer.writeEndElement(); // Target

    location().serializeToXml(writer);

    writer.writeStartElement(C::xmlKeyInterpolation());
    writer.writeAttribute(C::xmlKeyLine0(), QString::number(m_interpolation.line0));
    writer.writeAttribute(C::xmlKeyLine1(), QString::number(m_interpolation.line1));
    writer.writeAttribute(C::xmlKeyValue0(), positionToXml(m_interpolation.value0));
    writer.writeAttribute(C::xmlKeyValue1(), positionToXml(m_interpolation.value1));
    if (m_interpolation.curve != Interpolator::CurveType::Linear) {
        writer.writeAttribute(C::xmlKeyCurve(), Interpolator::curveToXmlValue(m_interpolation.curve));
    }
    writer.writeEndElement(); // Interpolation

    if (m_modulation.cycles > 0.f || m_modulation.amplitude > 0.f || m_modulation.offset != 0.f) {
        writer.writeStartElement(C::xmlKeyModulation());
        writer.writeAttribute(C::xmlKeyType(), m_modulation.type == ModulationParameters::ModulationType::SineWave ? C::xmlValueSineWave() : C::xmlValueRandom());
        writer.writeAttribute(C::xmlKeyCycles(), QString::number(static_cast<int>(m_modulation.cycles)));
        writer.writeAttribute(C::xmlKeyAmplitude(), QString::number(static_cast<int>(m_modulation.amplitude)));
        writer.writeAttribute(C::xmlKeyOffset(), QString::number(static_cast<int>(m_modulation.offset)));
        writer.writeAttribute(C::xmlKeyInverted(), m_modulation.inverted ? C::xmlValueTrue() : C::xmlValueFalse());
        writer.writeEndElement(); // Modulation
    }

    writer.writeEndElement(); // ParameterAutomation
}

ParameterAutomation::ParameterAutomationU ParameterAutomation::deserializeFromXml(ProjectReader & reader)
{
    const auto mode = reader.attribute(C::xmlKeyMode()).toString() == C::xmlValueRelative() ? Mode::Relative : Mode::Absolute;
    const auto eventsPerBeat = static_cast<uint8_t>(reader.attribute(C::xmlKeyEventsPerBeat()).toUInt());
    const auto lineOffset = static_cast<uint8_t>(reader.attribute(C::xmlKeyLineOffset()).toUInt());
    const auto comment = reader.attribute(C::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, C::xmlKeyEnabled(), false).value_or(true);

    Target target;
    auto location = std::make_unique<AutomationLocation>();
    InterpolationParameters interpolation;
    ModulationParameters modulation;

    while (!(reader.isEndElement() && !reader.name().compare(C::xmlKeyParameterAutomation()))) {
        reader.readNext();
        if (!reader.isStartElement()) {
            continue;
        }
        if (!reader.name().compare(C::xmlKeyAutomationTarget())) {
            target.owner = Target::Owner::Effect;
            target.ownerId = reader.attribute(C::xmlKeyOwnerId()).toString().toStdString();
            target.parameter = reader.attribute(C::xmlKeyParameterName()).toString().toStdString();
        } else if (!reader.name().compare(C::xmlKeyLocation())) {
            location = AutomationLocation::deserializeFromXml(reader);
        } else if (!reader.name().compare(C::xmlKeyInterpolation())) {
            interpolation.line0 = reader.attribute(C::xmlKeyLine0()).toULongLong();
            interpolation.line1 = reader.attribute(C::xmlKeyLine1()).toULongLong();
            interpolation.value0 = reader.attribute(C::xmlKeyValue0()).toDouble();
            interpolation.value1 = reader.attribute(C::xmlKeyValue1()).toDouble();
            if (const auto curve = reader.attribute(C::xmlKeyCurve()); curve.isValid()) {
                interpolation.curve = Interpolator::curveFromXmlValue(curve.toString());
            }
        } else if (!reader.name().compare(C::xmlKeyModulation())) {
            modulation.type = reader.attribute(C::xmlKeyType()).toString() == C::xmlValueRandom() ? ModulationParameters::ModulationType::Random : ModulationParameters::ModulationType::SineWave;
            modulation.cycles = static_cast<float>(reader.attribute(C::xmlKeyCycles()).toInt());
            modulation.amplitude = static_cast<float>(reader.attribute(C::xmlKeyAmplitude()).toInt());
            modulation.offset = static_cast<float>(reader.attribute(C::xmlKeyOffset()).toInt());
            modulation.inverted = Utils::Xml::readBoolAttribute(reader, C::xmlKeyInverted(), false).value_or(false);
        }
    }

    auto automation = std::make_unique<ParameterAutomation>(0, *location, target, mode, interpolation, modulation, comment, enabled);
    automation->setEventsPerBeat(eventsPerBeat);
    automation->setLineOffset(lineOffset);
    return automation;
}

} // namespace noteahead
