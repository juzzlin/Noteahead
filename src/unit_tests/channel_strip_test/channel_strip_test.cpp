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

#include "channel_strip_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>

#include <cmath>
#include <memory>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 256 };
//! Buffers rendered and discarded before measuring, so the note's attack has passed.
constexpr int WarmUpBuffers { 8 };

//! Hard-clips at a level the fixture crosses, so where the fader sits relative to it is audible.
//! A stand-in for the real dynamics effects, but with no parameters to drift underneath the test.
class ClippingEffect : public Effect
{
public:
    std::string type() const override
    {
        return "TestClipper";
    }

    std::string typeId() const override
    {
        return "test-clipper";
    }

    void processSample(double & left, double & right) override
    {
        left = std::clamp(left, -Threshold, Threshold);
        right = std::clamp(right, -Threshold, Threshold);
    }

private:
    static constexpr double Threshold { 0.02 };
};

//! Halves the signal. Linear, so a scalar fader commutes with it exactly.
class HalvingEffect : public Effect
{
public:
    std::string type() const override
    {
        return "TestHalver";
    }

    std::string typeId() const override
    {
        return "test-halver";
    }

    void processSample(double & left, double & right) override
    {
        left *= 0.5;
        right *= 0.5;
    }
};

using DeviceUnderTest = StringEnsembleDevice;

std::unique_ptr<DeviceUnderTest> makeDevice(float volume)
{
    auto device = std::make_unique<DeviceUnderTest>("Fixture");
    device->setSampleRate(SampleRate);
    device->setVolume(volume);
    device->setCrescendo(0.0f);
    // The ensemble chorus runs a 25 ms delay line and would otherwise dominate what these tests
    // measure; the strip is what is under test here, not the chorus.
    device->setModulationEnabled(false);
    device->processMidiNoteOn(60, 127);
    return device;
}

//! Renders one buffer of the device alone, with the fader and inserts left to the caller.
std::vector<double> renderRaw(Device & device)
{
    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
    device.processAudio(context);
    return samples;
}

//! Renders one buffer the way the audio engine does, honouring the device's fader position.
std::vector<double> renderThroughStrip(Device & device)
{
    for (int i = 0; i < WarmUpBuffers; i++) {
        renderRaw(device);
    }
    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
    device.processAudio(context);
    if (device.faderPosition() == Device::FaderPosition::PreInserts) {
        device.applyFader(context);
        device.processInsertEffects(context);
    } else {
        device.processInsertEffects(context);
        device.applyFader(context);
    }
    return samples;
}

//! Renders one buffer and returns the signal as it stood at the send tap.
std::vector<double> renderToSendTap(Device & device)
{
    for (int i = 0; i < WarmUpBuffers; i++) {
        renderRaw(device);
    }
    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
    device.processAudio(context);
    device.processInsertEffects(context);
    if (device.sendTap() == Device::SendTap::PreFader) {
        return samples;
    }
    device.applyFader(context);
    return samples;
}

double peakLevel(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (const auto sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

double maximumDifference(const std::vector<double> & a, const std::vector<double> & b)
{
    double worst = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        worst = std::max(worst, std::abs(a[i] - b[i]));
    }
    return worst;
}

} // namespace

void ChannelStripTest::test_defaults_shouldMatchLegacyChain()
{
    // The defaults are what keeps projects saved before the setting existed sounding the same.
    const DeviceUnderTest device { "Fixture" };
    QCOMPARE(static_cast<int>(device.faderPosition()), static_cast<int>(Device::FaderPosition::PreInserts));
    QCOMPARE(static_cast<int>(device.sendTap()), static_cast<int>(Device::SendTap::PostFader));
}

void ChannelStripTest::test_applyFader_shouldScaleTheWholeBuffer()
{
    constexpr float position = 0.25f;
    auto device = makeDevice(position);
    for (int i = 0; i < WarmUpBuffers; i++) {
        renderRaw(*device);
    }

    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
    device->processAudio(context);
    const auto dry = samples;
    device->applyFader(context);

    QVERIFY(peakLevel(dry) > 0.0);
    for (size_t i = 0; i < samples.size(); i++) {
        QVERIFY(std::abs(samples[i] - dry[i] * ParameterMapper::mapFader(position)) < 1.0e-12);
    }
}

void ChannelStripTest::test_applyFader_unityShouldLeaveBufferUntouched()
{
    auto device = makeDevice(Constants::faderUnityPosition());
    for (int i = 0; i < WarmUpBuffers; i++) {
        renderRaw(*device);
    }

    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
    device->processAudio(context);
    const auto dry = samples;
    QVERIFY(peakLevel(dry) > 0.0);
    device->applyFader(context);

    QCOMPARE(samples, dry);
}

void ChannelStripTest::test_faderPosition_nonLinearInsert_shouldDifferBetweenOrderings()
{
    auto legacy = makeDevice(0.25f);
    legacy->insertEffectRack().setEffect(0, std::make_shared<ClippingEffect>());
    legacy->setFaderPosition(Device::FaderPosition::PreInserts);

    auto modern = makeDevice(0.25f);
    modern->insertEffectRack().setEffect(0, std::make_shared<ClippingEffect>());
    modern->setFaderPosition(Device::FaderPosition::PostInserts);

    const auto legacySamples = renderThroughStrip(*legacy);
    const auto modernSamples = renderThroughStrip(*modern);

    // Pulling the fader down before a clipper means less of the signal reaches its threshold, which
    // is exactly the coupling between balance and dynamics the new ordering removes.
    QVERIFY(maximumDifference(legacySamples, modernSamples) > 1.0e-6);
}

void ChannelStripTest::test_faderPosition_linearInsert_shouldAgreeBetweenOrderings()
{
    auto legacy = makeDevice(0.25f);
    legacy->insertEffectRack().setEffect(0, std::make_shared<HalvingEffect>());
    legacy->setFaderPosition(Device::FaderPosition::PreInserts);

    auto modern = makeDevice(0.25f);
    modern->insertEffectRack().setEffect(0, std::make_shared<HalvingEffect>());
    modern->setFaderPosition(Device::FaderPosition::PostInserts);

    const auto legacySamples = renderThroughStrip(*legacy);
    const auto modernSamples = renderThroughStrip(*modern);

    QVERIFY(peakLevel(legacySamples) > 0.0);
    QVERIFY(maximumDifference(legacySamples, modernSamples) < 1.0e-12);
}

void ChannelStripTest::test_faderPosition_postInserts_shouldNotChangeInsertInput()
{
    // The point of the whole change: with the fader after the inserts, moving it must not alter what
    // the inserts are fed, so a clipper keeps clipping identically.
    const auto renderAtVolume = [](float volume) {
        auto device = makeDevice(volume);
        device->setFaderPosition(Device::FaderPosition::PostInserts);
        for (int i = 0; i < WarmUpBuffers; i++) {
            renderRaw(*device);
        }
        std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
        AudioContext context { std::span(samples.data(), samples.size()), FrameCount, SampleRate };
        device->processAudio(context);
        device->processInsertEffects(context);
        return samples;
    };

    const auto loud = renderAtVolume(1.0f);
    const auto quiet = renderAtVolume(0.1f);

    QVERIFY(peakLevel(loud) > 0.0);
    QCOMPARE(loud, quiet);
}

void ChannelStripTest::test_sendTap_preFader_shouldIgnoreFader()
{
    const auto renderAtVolume = [](float volume) {
        auto device = makeDevice(volume);
        device->setSendTap(Device::SendTap::PreFader);
        return renderToSendTap(*device);
    };

    const auto loud = renderAtVolume(1.0f);
    const auto quiet = renderAtVolume(0.2f);

    QVERIFY(peakLevel(loud) > 0.0);
    QCOMPARE(loud, quiet);
}

void ChannelStripTest::test_sendTap_postFader_shouldFollowFader()
{
    const auto renderAtVolume = [](float volume) {
        auto device = makeDevice(volume);
        device->setSendTap(Device::SendTap::PostFader);
        return renderToSendTap(*device);
    };

    const auto loud = renderAtVolume(Constants::faderUnityPosition());
    const auto quiet = renderAtVolume(0.2f);

    QVERIFY(peakLevel(loud) > 0.0);
    // The fader is stored as a float, so compare against the value the device actually holds
    // rather than the double literal it was set from.
    const double expected = peakLevel(loud) * ParameterMapper::mapFader(static_cast<double>(0.2f));
    QVERIFY2(std::abs(peakLevel(quiet) - expected) < 1.0e-12,
             qPrintable(QString { "expected %1, got %2" }.arg(expected).arg(peakLevel(quiet))));
}

void ChannelStripTest::test_settings_shouldRoundTripThroughXml()
{
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);

    {
        NahdXmlWriter writer { buffer };
        DeviceUnderTest device { "Fixture" };
        device.setFaderPosition(Device::FaderPosition::PostInserts);
        device.setSendTap(Device::SendTap::PreFader);
        device.serializeToXml(writer);
    }

    buffer.close();
    buffer.open(QIODevice::ReadOnly);

    NahdXmlReader reader { buffer };
    DeviceUnderTest device { "Restored" };
    QVERIFY(reader.readNextStartElement());
    device.deserializeFromXml(reader);

    QCOMPARE(static_cast<int>(device.faderPosition()), static_cast<int>(Device::FaderPosition::PostInserts));
    QCOMPARE(static_cast<int>(device.sendTap()), static_cast<int>(Device::SendTap::PreFader));
}

void ChannelStripTest::test_settings_absentFromXml_shouldLoadAsLegacy()
{
    // A project written before the settings existed carries neither key, and must come back with
    // the chain it was mixed through.
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);

    {
        NahdXmlWriter writer { buffer };
        DeviceUnderTest device { "Fixture" };
        device.setFaderPosition(Device::FaderPosition::PostInserts);
        device.setSendTap(Device::SendTap::PreFader);
        device.serializeToXml(writer);
    }
    buffer.close();

    // Strip the two parameter elements back out, leaving the document an older Noteahead would
    // have written.
    const auto stripParameter = [](QByteArray & xml, const QString & key) {
        const auto marker = QString { "name=\"%1\"" }.arg(key).toUtf8();
        const auto at = xml.indexOf(marker);
        QVERIFY2(at >= 0, qPrintable(QString { "%1 was never written" }.arg(key)));
        const auto start = xml.lastIndexOf('<', at);
        const auto end = xml.indexOf('>', at);
        xml.remove(start, end - start + 1);
    };
    stripParameter(data, Constants::NahdXml::xmlKeyFaderPosition());
    stripParameter(data, Constants::NahdXml::xmlKeySendTap());
    QVERIFY(!data.contains(Constants::NahdXml::xmlKeyFaderPosition().toUtf8()));
    QVERIFY(!data.contains(Constants::NahdXml::xmlKeySendTap().toUtf8()));

    buffer.open(QIODevice::ReadOnly);

    NahdXmlReader reader { buffer };
    DeviceUnderTest device { "Restored" };
    QVERIFY(reader.readNextStartElement());
    device.deserializeFromXml(reader);

    QCOMPARE(static_cast<int>(device.faderPosition()), static_cast<int>(Device::FaderPosition::PreInserts));
    QCOMPARE(static_cast<int>(device.sendTap()), static_cast<int>(Device::SendTap::PostFader));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ChannelStripTest)
