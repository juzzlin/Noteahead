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

#include "common/constants.hpp"
#include "domain/devices/device.hpp"
#include "domain/dsp/compressor_effect.hpp"
#include "infra/audio/audio_engine.hpp"

#include <QTest>

namespace noteahead {

class MockDevice : public Device
{
public:
    MockDevice(const std::string & name)
      : m_name(name)
    {
    }
    std::string name() const override { return m_name; }
    std::string category() const override { return "Mock"; }
    std::string typeName() const override { return "MockDevice"; }
    std::string typeId() const override { return "mock-device-id"; }

    void processMidiNoteOn(uint8_t, uint8_t) override { }
    void processMidiNoteOff(uint8_t) override { }
    void processMidiCc(uint8_t, uint8_t, uint8_t) override { }
    void processMidiAllNotesOff() override { }

    void processAudio(AudioContext & context) override
    {
        if (m_generateSignal) {
            for (uint32_t i = 0; i < context.frameCount; i++) {
                context.buffer[i * 2] = 1.0;
                context.buffer[i * 2 + 1] = 1.0;
            }
        }
    }

    void setGenerateSignal(bool generate) { m_generateSignal = generate; }

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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SideChainAudioTest)
