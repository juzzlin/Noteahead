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

#include "fader_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

//! Serialises a device parameter exactly the way a project saved before the boost range existed
//! did: under the old "volume" name, and carrying the range that was in effect back then.
QByteArray legacyDeviceXml(int storedValue)
{
    QByteArray data;
    NahdXmlWriter writer { data };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), Constants::NahdXml::xmlKeyVolume());
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), QString::number(storedValue));
    writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), "0");
    writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), "10000");
    writer.writeAttribute(Constants::NahdXml::xmlKeyDefault(), "10000");
    writer.writeAttribute(Constants::NahdXml::xmlKeyScale(), "100");
    writer.writeEndElement(); // Parameter
    writer.writeEndElement(); // Parameters
    writer.writeEndElement(); // Device
    return data;
}

//! Loads XML into a device through the same path a project load takes.
void load(Device & device, const QByteArray & data)
{
    NahdXmlReader reader { data };
    while (!reader.atEnd() && !reader.isStartElement()) {
        reader.readNext();
    }
    device.deserializeFromXml(reader);
}

//! The gain the device's fader currently applies.
double gainOf(const Device & device)
{
    return ParameterMapper::mapFader(static_cast<double>(device.volume()));
}

} // namespace

void FaderTest::test_mapFader_unityPosition_shouldGiveUnityGain()
{
    QVERIFY(std::abs(ParameterMapper::mapFader(Constants::faderUnityPosition()) - 1.0) < 1.0e-12);
}

void FaderTest::test_mapFader_fullThrow_shouldGiveMaximumBoost()
{
    const double expected = std::pow(10.0, Constants::maxFaderBoostDb() / 20.0);
    QVERIFY2(std::abs(ParameterMapper::mapFader(1.0) - expected) < 1.0e-12,
             qPrintable(QString { "Expected %1, got %2" }.arg(expected).arg(ParameterMapper::mapFader(1.0))));
}

void FaderTest::test_mapFader_belowUnity_shouldKeepLinearAmplitudeTaper()
{
    // Half the throw up to unity is half the amplitude, which is what the fader did before the
    // boost range existed. Only where that lands on the knob has changed.
    const double half = ParameterMapper::mapFader(Constants::faderUnityPosition() / 2.0);
    QVERIFY2(std::abs(half - 0.5) < 1.0e-12, qPrintable(QString { "Got %1" }.arg(half)));
}

void FaderTest::test_mapFader_zero_shouldBeSilent()
{
    QCOMPARE(ParameterMapper::mapFader(0.0), 0.0);
}

void FaderTest::test_mapFader_shouldRoundTripThroughUnmap()
{
    for (const double position : { 0.0, 0.1, 0.375, 0.75, 0.9, 1.0 }) {
        const double roundTripped = ParameterMapper::unmapFader(ParameterMapper::mapFader(position));
        QVERIFY2(std::abs(roundTripped - position) < 1.0e-9,
                 qPrintable(QString { "Position %1 round tripped to %2" }.arg(position).arg(roundTripped)));
    }
}

void FaderTest::test_legacyVolume_fullScale_shouldLoadAsUnityGain()
{
    // The trap this conversion exists for: the stored 10000 is full scale of the *old* range, and
    // full scale of the new one is +10 dB. Read naively, every device in every old project would
    // come back 10 dB hot.
    PianoSynthDevice device { "Fixture" };
    load(device, legacyDeviceXml(10000));

    QVERIFY2(std::abs(gainOf(device) - 1.0) < 1.0e-6, qPrintable(QString { "Gain %1" }.arg(gainOf(device))));
    QVERIFY(std::abs(device.volume() - Constants::faderUnityPosition()) < 1.0e-6f);
}

void FaderTest::test_legacyVolume_halfScale_shouldPreserveGain()
{
    PianoSynthDevice device { "Fixture" };
    load(device, legacyDeviceXml(5000));

    QVERIFY2(std::abs(gainOf(device) - 0.5) < 1.0e-6, qPrintable(QString { "Gain %1" }.arg(gainOf(device))));
}

void FaderTest::test_legacyVolume_zero_shouldPreserveSilence()
{
    PianoSynthDevice device { "Fixture" };
    load(device, legacyDeviceXml(0));

    QCOMPARE(gainOf(device), 0.0);
}

void FaderTest::test_legacyVolume_samplerPad_shouldPreserveGain()
{
    // Pads carry their own fader, stored under the same legacy name, so they need the same conversion
    SamplerDevice::Sample pad;

    const auto data = legacyDeviceXml(5000);
    NahdXmlReader reader { data };
    while (!reader.atEnd() && reader.name() != Constants::NahdXml::xmlKeyParameters()) {
        reader.readNext();
    }
    pad.deserializeParametersFromXml(reader);

    const auto p = pad.parameter(Constants::NahdXml::xmlKeyFader().toStdString());
    QVERIFY(p);
    const double gain = ParameterMapper::mapFader(static_cast<double>(p->get().value()));
    QVERIFY2(std::abs(gain - 0.5) < 1.0e-6, qPrintable(QString { "Gain %1" }.arg(gain)));
}

void FaderTest::test_legacyVolume_absent_shouldDefaultToUnityGain()
{
    // A project old enough to carry no fader at all still has to come back at unity
    PianoSynthDevice device { "Fixture" };
    QVERIFY(std::abs(gainOf(device) - 1.0) < 1.0e-6);
}

void FaderTest::test_fader_boostedPosition_shouldRoundTripThroughXml()
{
    QByteArray data;
    {
        PianoSynthDevice device { "Fixture" };
        device.setVolume(1.0f); // Top of the throw, so the boost range is what round trips
        NahdXmlWriter writer { data };
        device.serializeToXml(writer);
    }

    PianoSynthDevice device { "Fixture" };
    load(device, data);

    QCOMPARE(device.volume(), 1.0f);
    const double expected = std::pow(10.0, Constants::maxFaderBoostDb() / 20.0);
    QVERIFY2(std::abs(gainOf(device) - expected) < 1.0e-6, qPrintable(QString { "Gain %1" }.arg(gainOf(device))));
}

void FaderTest::test_midiCc7_fullValue_shouldLandOnUnity()
{
    // CC 7 at 127 is nominal maximum, not "boost by 10 dB", or existing CC 7 automation would get
    // louder just by opening the project in a newer build.
    PianoSynthDevice device { "Fixture" };
    device.processMidiCc(7, 127, 0);

    QCOMPARE(device.volume(), Constants::faderUnityPosition());
    QVERIFY(std::abs(gainOf(device) - 1.0) < 1.0e-6);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FaderTest)
