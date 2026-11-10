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
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "parameter.hpp"

#include <QVariant>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <string_view>
#include <tuple>

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

QString ParameterAutomation::Target::ownerToString(Owner owner)
{
    switch (owner) {
    case Owner::Device:
        return C::xmlValueDevice();
    case Owner::Midi:
        return C::xmlValueMidi();
    case Owner::Effect:
        break;
    }
    return C::xmlValueEffect();
}

ParameterAutomation::Target::Owner ParameterAutomation::Target::ownerFromString(const QString & owner)
{
    if (owner == C::xmlValueDevice()) {
        return Owner::Device;
    }
    return owner == C::xmlValueMidi() ? Owner::Midi : Owner::Effect;
}

bool ParameterAutomation::Target::isSameDestination(const Target & other) const
{
    return owner == other.owner && ownerId == other.ownerId && parameter == other.parameter;
}

ParameterAutomation::Target ParameterAutomation::Target::withRangeOf(const Target & previous) const
{
    auto target = *this;
    if (const auto address = midiAddress(); address) {
        if (const auto previousAddress = previous.midiAddress(); previousAddress && previousAddress->message == address->message) {
            target.midiMin = previous.midiMin;
            target.midiMax = previous.midiMax;
        } else {
            std::tie(target.midiMin, target.midiMax) = fullRange(address->message);
        }
    }
    return target;
}

std::pair<double, double> ParameterAutomation::Target::fullRange(MidiMessage message)
{
    return message == MidiMessage::PitchBend ? std::pair { -1.0, 1.0 } : std::pair { 0.0, 1.0 };
}

bool ParameterAutomation::Target::hasPartialMidiRange() const
{
    const auto address = midiAddress();
    return address && std::pair { midiMin, midiMax } != fullRange(address->message);
}

void ParameterAutomation::Target::setMidiRange(std::optional<double> min, std::optional<double> max)
{
    const auto address = midiAddress();
    const auto [lowest, highest] = fullRange(address ? address->message : MidiMessage::Cc);
    midiMin = std::clamp(min.value_or(lowest), lowest, highest);
    midiMax = std::clamp(max.value_or(highest), lowest, highest);
}

namespace {
double displayScale(const std::optional<ParameterAutomation::Target::MidiAddress> & address)
{
    return address && address->message == ParameterAutomation::Target::MidiMessage::PitchBend ? 100.0 : 127.0;
}
} // namespace

int ParameterAutomation::Target::displayedMidiValue(double value) const
{
    return static_cast<int>(std::lround(value * displayScale(midiAddress())));
}

double ParameterAutomation::Target::midiValueFromDisplayed(int displayed) const
{
    return static_cast<double>(displayed) / displayScale(midiAddress());
}

std::string ParameterAutomation::Target::midiCcKey(uint8_t channel, uint8_t controller)
{
    return std::format("{}:{}", channel, controller);
}

std::string ParameterAutomation::Target::midiPitchBendKey(uint8_t channel)
{
    return std::format("{}:{}", channel, C::xmlValuePitchBendKey().toStdString());
}

std::optional<ParameterAutomation::Target::MidiAddress> ParameterAutomation::Target::midiAddress() const
{
    if (owner != Owner::Midi) {
        return std::nullopt;
    }
    const auto separator = parameter.find(':');
    if (separator == std::string::npos) {
        return std::nullopt;
    }
    const auto parse = [](std::string_view text) -> std::optional<unsigned> {
        unsigned number = 0;
        if (const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), number); error != std::errc {} || end != text.data() + text.size()) {
            return std::nullopt;
        }
        return number;
    };
    const auto channel = parse(std::string_view { parameter }.substr(0, separator));
    if (!channel || *channel > 15) {
        return std::nullopt;
    }
    const auto message = std::string_view { parameter }.substr(separator + 1);
    if (message == C::xmlValuePitchBendKey().toStdString()) {
        return MidiAddress { static_cast<uint8_t>(*channel), MidiMessage::PitchBend, 0 };
    }
    const auto controller = parse(message);
    if (!controller || *controller > 127) {
        return std::nullopt;
    }
    return MidiAddress { static_cast<uint8_t>(*channel), MidiMessage::Cc, static_cast<uint8_t>(*controller) };
}

double ParameterAutomation::Target::midiValue(double position) const
{
    const auto address = midiAddress();
    const auto [lowest, highest] = fullRange(address ? address->message : MidiMessage::Cc);
    const double low = std::clamp(midiMin, lowest, highest);
    const double high = std::clamp(midiMax, lowest, highest);
    return low + std::clamp(position, 0.0, 1.0) * (high - low);
}

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

float ParameterAutomation::effectiveValue(Mode mode, double value, const Parameter & parameter)
{
    const double position = std::clamp(value, 0.0, 1.0);
    if (mode == Mode::Absolute) {
        return static_cast<float>(position);
    }
    const double knob = static_cast<double>(parameter.authoredValue());
    // A fader scales its gain rather than its throw, so that relative is exactly what Expression is
    // over MIDI: half is half the amplitude of wherever the fader stands. Scaling the position
    // instead would follow the taper, and half of the throw is a good deal quieter than half.
    if (parameter.display() && parameter.display()->mapping == ParameterDisplay::Mapping::Fader) {
        return static_cast<float>(ParameterMapper::unmapFader(ParameterMapper::mapFader(knob) * position));
    }
    const double rest = parameter.display() ? parameter.display()->restPosition() : 0.0;
    return static_cast<float>(rest + (knob - rest) * position);
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
    writer.writeAttribute(C::xmlKeyOwner(), Target::ownerToString(m_target.owner));
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
    if (m_interpolation.curve == Interpolator::CurveType::Drawn) {
        m_interpolation.drawnCurve.serializeToXml(writer);
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
            // Effects came first, so an automation that does not say is aimed at one
            target.owner = Target::ownerFromString(reader.attribute(C::xmlKeyOwner()).toString());
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
        } else if (!reader.name().compare(C::xmlKeyNode())) {
            interpolation.drawnCurve.deserializeNodeFromXml(reader);
        } else if (!reader.name().compare(C::xmlKeyModulation())) {
            modulation.type = reader.attribute(C::xmlKeyType()).toString() == C::xmlValueRandom() ? ModulationParameters::ModulationType::Random : ModulationParameters::ModulationType::SineWave;
            modulation.cycles = static_cast<float>(reader.attribute(C::xmlKeyCycles()).toInt());
            modulation.amplitude = static_cast<float>(reader.attribute(C::xmlKeyAmplitude()).toInt());
            modulation.offset = static_cast<float>(reader.attribute(C::xmlKeyOffset()).toInt());
            modulation.inverted = Utils::Xml::readBoolAttribute(reader, C::xmlKeyInverted(), false).value_or(false);
        }
    }

    // A drawn curve without nodes, which this version never writes, plays as the ramp between its ends
    if (interpolation.curve == Interpolator::CurveType::Drawn && interpolation.drawnCurve.isEmpty()) {
        interpolation.drawnCurve = AutomationCurve::ramp(interpolation.line0, static_cast<double>(interpolation.value0), interpolation.line1, static_cast<double>(interpolation.value1));
    }
    auto automation = std::make_unique<ParameterAutomation>(0, *location, target, mode, interpolation, modulation, comment, enabled);
    automation->setEventsPerBeat(eventsPerBeat);
    automation->setLineOffset(lineOffset);
    return automation;
}

} // namespace noteahead
