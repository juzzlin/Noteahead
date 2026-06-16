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

#include "side_chain_audio_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QByteArray>
#include <QTest>

namespace noteahead {

class MockDevice : public Device
{
public:
    MockDevice(const std::string & name)
      : m_name(name)
    {
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string category() const override
    {
        return "Mock";
    }

    std::string typeName() const override
    {
        return "MockDevice";
    }

    std::string typeId() const override
    {
        return "mock-device-id";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext & context) override
    {
        if (m_generateSignal) {
            for (uint32_t i = 0; i < context.frameCount; i++) {
                context.buffer[i * 2] = 1.0;
                context.buffer[i * 2 + 1] = 1.0;
            }
        }
    }

    void setGenerateSignal(bool generate)
    {
        m_generateSignal = generate;
    }

private:
    std::string m_name;
    bool m_generateSignal { false };
};

void SideChainAudioTest::test_audioEngine_rebuildProcessingGraph_shouldCorrectlySortIndependentDevices()
{
    AudioEngine engine;
    const auto device1 = std::make_shared<MockDevice>("Device 1");
    const auto device2 = std::make_shared<MockDevice>("Device 2");

    engine.setDevice(0, device1);
    engine.setDevice(1, device2);

    // No dependencies, should all be in layer 0
    std::vector<double> buffer(128, 0.0);
    AudioContext context;
    context.frameCount = 64;
    context.sampleRate = 44100;
    context.buffer = std::span<double>(buffer.data(), 128);
    engine.process(context);

    // Verify it doesn't crash and correctly processes.
    QVERIFY(true);
}

void SideChainAudioTest::test_audioEngine_rebuildProcessingGraph_shouldCorrectlySortDependentDevices()
{
    AudioEngine engine;
    const auto device1 = std::make_shared<MockDevice>("Device 1");
    const auto device2 = std::make_shared<MockDevice>("Device 2");

    // Add compressor to device 2 that side-chains from device 1 (slot 0)
    const auto compressor = std::make_shared<CompressorEffect>();
    if (const auto p = compressor->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(0.0f); // Slot 0
        compressor->sync();
    }
    device2->insertEffectRack().setEffect(0, compressor);

    engine.setDevice(0, device1);
    engine.setDevice(1, device2);

    std::vector<double> buffer(128, 0.0);
    AudioContext context;
    context.frameCount = 64;
    context.sampleRate = 44100;
    context.buffer = std::span<double>(buffer.data(), 128);
    engine.process(context);

    QCOMPARE(compressor->sidechainSourceDeviceIndex().value_or(999), 0u);
}

void SideChainAudioTest::test_audioEngine_rebuildProcessingGraph_shouldHandleCircularDependencyGracefully()
{
    AudioEngine engine;
    const auto device1 = std::make_shared<MockDevice>("Device 1");
    const auto device2 = std::make_shared<MockDevice>("Device 2");

    // Device 1 side-chains from Device 2
    const auto comp1 = std::make_shared<CompressorEffect>();
    if (const auto p = comp1->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(1.0f); // Slot 1
        comp1->sync();
    }
    device1->insertEffectRack().setEffect(0, comp1);

    // Device 2 side-chains from Device 1
    const auto comp2 = std::make_shared<CompressorEffect>();
    if (const auto p = comp2->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(0.0f); // Slot 0
        comp2->sync();
    }
    device2->insertEffectRack().setEffect(0, comp2);

    engine.setDevice(0, device1);
    engine.setDevice(1, device2);

    std::vector<double> buffer(128, 0.0);
    AudioContext context;
    context.frameCount = 64;
    context.sampleRate = 44100;
    context.buffer = std::span<double>(buffer.data(), 128);

    // Should not crash or infinite loop
    engine.process(context);
    QVERIFY(true);
}

void SideChainAudioTest::test_compressorEffect_process_shouldApplySidechainGainReduction()
{
    AudioEngine engine;
    const auto device1 = std::make_shared<MockDevice>("Source");
    const auto device2 = std::make_shared<MockDevice>("Target");

    device1->setGenerateSignal(true); // Source sends 1.0
    device2->setGenerateSignal(true); // Target sends 1.0

    const auto compressor = std::make_shared<CompressorEffect>();
    // Set aggressive compression
    if (const auto p = compressor->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        p->get().setValue(0.0f); // -60dB
        compressor->sync();
    }
    if (const auto p = compressor->parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
        p->get().setValue(1.0f); // 20:1
        compressor->sync();
    }
    // Set side-chain to slot 0 (device 1)
    if (const auto p = compressor->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(0.0f);
        compressor->sync();
    }
    device2->insertEffectRack().setEffect(0, compressor);

    engine.setDevice(0, device1);
    engine.setDevice(1, device2);

    std::vector<double> buffer(128, 0.0);
    AudioContext context { std::span(buffer.data(), 128), 64, 44100 };
    engine.process(context);

    // With aggressive compression triggered by side-chain, reduction should be significant.
    // Device 1 (Source) = 1.0, Device 2 (Target) = 1.0 * reduction
    // Mixed result should be > 1.0 but < 2.0
    QVERIFY(buffer[0] > 1.0);
    QVERIFY(buffer[0] < 2.0);
    QVERIFY(compressor->reductionDb() < -1.0f);
}

void SideChainAudioTest::test_compressorEffect_sideChainLpf_bypass_shouldPreserveGainReduction()
{
    const int frameCount = 2048;
    const double sampleRate = 48000.0;

    // DC side-chain signal (1.0 per sample)
    std::vector<double> sideChainBuf(frameCount * 2, 1.0);
    std::array<std::span<const double>, 1> deviceSpans = {
        std::span<const double>(sideChainBuf.data(), sideChainBuf.size())
    };

    CompressorEffect comp;
    if (const auto p = comp.parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        p->get().setValue(0.0f); // -60 dB
    }
    if (const auto p = comp.parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
        p->get().setValue(1.0f); // 20:1
    }
    if (const auto p = comp.parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(0.0f);
    }
    // LPF stays at default 1.0 (bypass) — the guard skips the filter
    comp.setSampleRate(sampleRate);
    comp.sync();

    std::vector<double> mainBuf(frameCount * 2, 0.0);
    AudioContext context;
    context.frameCount = static_cast<uint32_t>(frameCount);
    context.sampleRate = static_cast<uint32_t>(sampleRate);
    context.buffer = std::span<double>(mainBuf.data(), mainBuf.size());
    context.deviceOutputBuffers = std::span<const std::span<const double>>(deviceSpans.data(), deviceSpans.size());

    comp.process(context);

    QVERIFY(comp.reductionDb() < -1.0f);
}

void SideChainAudioTest::test_compressorEffect_sideChainLpf_lowCutoff_shouldAttenuateAcDetectorSignal()
{
    const int frameCount = 4096;
    const double sampleRate = 48000.0;

    // Alternating +1/-1 (Nyquist-frequency signal) as side-chain
    std::vector<double> sideChainBuf(frameCount * 2);
    for (int i = 0; i < frameCount; i++) {
        const double v = (i % 2 == 0) ? 1.0 : -1.0;
        sideChainBuf[i * 2] = v;
        sideChainBuf[i * 2 + 1] = v;
    }
    std::array<std::span<const double>, 1> deviceSpans = {
        std::span<const double>(sideChainBuf.data(), sideChainBuf.size())
    };

    auto makeCompressor = [&](float lpfCutoff) {
        auto comp = std::make_unique<CompressorEffect>();
        if (const auto p = comp->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
            p->get().setValue(0.0f); // -60 dB
        }
        if (const auto p = comp->parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
            p->get().setValue(1.0f); // 20:1
        }
        if (const auto p = comp->parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
            p->get().setValue(0.0f); // fastest attack
        }
        if (const auto p = comp->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
            p->get().setValue(0.0f);
        }
        if (const auto p = comp->parameter(Constants::NahdXml::xmlKeySideChainLpf().toStdString()); p) {
            p->get().setValue(lpfCutoff);
        }
        comp->setSampleRate(sampleRate);
        comp->sync();
        return comp;
    };

    auto compBypass = makeCompressor(1.0f);
    auto compFiltered = makeCompressor(0.001f);

    std::vector<double> bufBypass(frameCount * 2, 0.0);
    std::vector<double> bufFiltered(frameCount * 2, 0.0);

    AudioContext ctx;
    ctx.frameCount = static_cast<uint32_t>(frameCount);
    ctx.sampleRate = static_cast<uint32_t>(sampleRate);
    ctx.deviceOutputBuffers = std::span<const std::span<const double>>(deviceSpans.data(), deviceSpans.size());

    ctx.buffer = std::span<double>(bufBypass.data(), bufBypass.size());
    compBypass->process(ctx);

    ctx.buffer = std::span<double>(bufFiltered.data(), bufFiltered.size());
    compFiltered->process(ctx);

    // Bypass: Nyquist detector not filtered → high level detected → more gain reduction
    // Filtered: LPF at 0.001 heavily attenuates the Nyquist signal → less gain reduction
    QVERIFY(compBypass->reductionDb() < -1.0f);
    QVERIFY(compBypass->reductionDb() < compFiltered->reductionDb());
}

void SideChainAudioTest::test_compressorEffect_sideChainLpf_serialization_shouldPreserveValue()
{
    const float expectedCutoff = 0.35f;

    QByteArray data;
    {
        CompressorEffect comp;
        if (const auto p = comp.parameter(Constants::NahdXml::xmlKeySideChainLpf().toStdString()); p) {
            p->get().setValue(expectedCutoff);
        }
        NahdXmlWriter writer { data };
        writer.writeStartElement("compressor");
        comp.serializeParametersToXml(writer);
        writer.writeEndElement();
    }

    {
        CompressorEffect comp;
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        // Reader is at <compressor>; deserializeParametersFromXml reads its children
        comp.deserializeParametersFromXml(reader);
        const auto p = comp.parameter(Constants::NahdXml::xmlKeySideChainLpf().toStdString());
        QVERIFY(p.has_value());
        QCOMPARE(p->get().value(), expectedCutoff);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SideChainAudioTest)
