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

#include "effect_rack_test.hpp"
#include "common/constants.hpp"
#include "domain/effects/effect_factory.hpp"
#include "domain/effects/effect_rack.hpp"
#include "domain/effects/volume_effect.hpp"
#include "domain/dsp/reverb_effect.hpp"

#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void EffectRackTest::initTestCase()
{
    EffectFactory::init();
}

void EffectRackTest::cleanupTestCase()
{
    EffectFactory::clear();
}

void EffectRackTest::test_addRemove_shouldAddAndRemoveEffects()
{
    EffectRack rack;
    QCOMPARE(rack.effectCount(), Constants::effectRackSize());

    auto reverb = std::make_shared<ReverbEffect>();
    rack.setEffect(0, reverb);
    QCOMPARE(rack.effectCount(), Constants::effectRackSize());
    QCOMPARE(rack.effect(0), reverb);

    rack.setEffect(0, nullptr);
    QCOMPARE(rack.effect(0), nullptr);
}

void EffectRackTest::test_process_shouldProcessAudio()
{
    EffectRack rack;
    auto reverb = std::make_shared<ReverbEffect>();
    reverb->setMix(1.0f); // Full wet
    reverb->setSize(0.5f);
    reverb->setDecay(0.5f);
    reverb->setPreDelay(0.0f);
    reverb->setLpfCutoff(1.0f);
    reverb->setHpfCutoff(0.0f);
    rack.setEffect(0, reverb);

    std::vector<double> output(2, 0.0);
    std::vector<double> sendBus(2, 1.0); // DC input
    AudioContext outputContext { std::span(output.data(), output.size()), 1, 44100 };

    // Reverb needs some samples to build up output
    for (int i = 0; i < 5000; i++) {

        rack.process(outputContext, sendBus.data(), 0);
    }

    QVERIFY(output[0] != 0.0f || output[1] != 0.0f);

    // Verify processing an empty slot doesn't crash
    std::vector<double> output2(2, 0.0);
    AudioContext outputContext2 { std::span(output2.data(), output2.size()), 1, 44100 };
    rack.process(outputContext2, sendBus.data(), 1);
    QCOMPARE(output2[0], 0.0f);
    QCOMPARE(output2[1], 0.0f);
}

void EffectRackTest::test_processInPlace_shouldApplyEffectToBuffer()
{
    EffectRack rack;
    auto volume = std::make_shared<VolumeEffect>();
    volume->setVolume(0.5f); // Half volume
    rack.setEffect(0, volume);

    std::vector<double> buffer(4, 1.0); // 2 stereo frames of DC 1.0
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, 44100 };

    rack.processInPlace(context);

    // All samples should be 0.5
    for (double sample : buffer) {
        QCOMPARE(sample, 0.5f);
    }
}

void EffectRackTest::test_serialization_shouldSerializeAndDeserializeEffects()
{
    EffectRack rack;
    auto reverb = std::make_shared<ReverbEffect>();
    reverb->setSize(0.75f);
    reverb->setLpfCutoff(0.7f);
    reverb->setHpfCutoff(0.25f);
    rack.setEffect(2, reverb);

    QString xml;
    QXmlStreamWriter writer(&xml);
    writer.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());
    rack.serializeEffectsToXml(writer);
    writer.writeEndElement();

    EffectRack rack2;
    QXmlStreamReader reader(xml);
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            rack2.deserializeEffectsFromXml(reader);
        } else {
            reader.skipCurrentElement();
        }
    }

    QCOMPARE(rack2.effectCount(), Constants::effectRackSize());
    QVERIFY(rack2.effect(0) == nullptr);
    QVERIFY(rack2.effect(1) == nullptr);
    auto reverb2 = std::dynamic_pointer_cast<ReverbEffect>(rack2.effect(2));
    QVERIFY(reverb2 != nullptr);
    QCOMPARE(reverb2->size(), 0.75f);
    QCOMPARE(reverb2->lpfCutoff(), 0.7f);
    QCOMPARE(reverb2->hpfCutoff(), 0.25f);
    QVERIFY(reverb2->enabled());

    // Test disabled serialization
    reverb2->setEnabled(false);
    xml.clear();
    QXmlStreamWriter writer2(&xml);
    writer2.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());
    rack2.serializeEffectsToXml(writer2);
    writer2.writeEndElement();

    EffectRack rack3;
    QXmlStreamReader reader2(xml);
    while (reader2.readNextStartElement()) {
        if (reader2.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            rack3.deserializeEffectsFromXml(reader2);
        } else {
            reader2.skipCurrentElement();
        }
    }
    auto reverb3 = std::dynamic_pointer_cast<ReverbEffect>(rack3.effect(2));
    QVERIFY(reverb3 != nullptr);
    QVERIFY(!reverb3->enabled());
}

void EffectRackTest::test_enabled_flag_shouldControlProcessing()
{
    EffectRack rack;
    auto volume = std::make_shared<VolumeEffect>();
    volume->setVolume(0.5f); // Half volume
    volume->setEnabled(false);
    rack.setEffect(0, volume);

    // Test processInPlace
    std::vector<double> buffer(4, 1.0); // 2 stereo frames of DC 1.0
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, 44100 };
    rack.processInPlace(context);

    // All samples should STILL be 1.0 because effect is disabled
    for (double sample : buffer) {
        QCOMPARE(sample, 1.0f);
    }

    // Test process (send bus style)
    std::vector<double> output(2, 0.0);
    std::vector<double> sendBus = { 1.0, 1.0 };
    AudioContext outputContext { std::span(output.data(), output.size()), 1, 44100 };
    rack.process(outputContext, sendBus.data(), 0);

    // Output should STILL be 0.0 because effect is disabled
    QCOMPARE(output[0], 0.0f);
    QCOMPARE(output[1], 0.0f);

    // Re-enable and verify it processes
    volume->setEnabled(true);
    rack.processInPlace(context);
    for (double sample : buffer) {
        QCOMPARE(sample, 0.5f);
    }
}

void EffectRackTest::test_reverb_parameters_shouldGetAndSetParameters()
{
    auto reverb = std::make_shared<ReverbEffect>();

    // Decay: 0.5 internal should be 5000ms
    reverb->setDecay(0.5f);
    QCOMPARE(reverb->decay(), 0.5f);

    // Pre-delay: 0.2 internal should be 100ms
    reverb->setPreDelay(0.2f);
    QCOMPARE(reverb->preDelay(), 0.2f);

    reverb->setLpfCutoff(0.65f);
    QCOMPARE(reverb->lpfCutoff(), 0.65f);

    reverb->setHpfCutoff(0.3f);
    QCOMPARE(reverb->hpfCutoff(), 0.3f);
}

void EffectRackTest::test_reverb_presets_shouldApplyPresets()
{
    auto reverb = std::make_shared<ReverbEffect>();

    const auto presets = ReverbEffect::presetNames();
    QVERIFY(!presets.empty());
    QCOMPARE(presets[0], "Hall");

    reverb->applyPreset(ReverbEffect::stringToPreset("Cathedral"));
    QCOMPARE(reverb->size(), 1.0f);
    QCOMPARE(reverb->lpfCutoff(), 0.84f);
    QCOMPARE(reverb->hpfCutoff(), 0.16f);

    QCOMPARE(ReverbEffect::presetToString(ReverbEffect::Preset::Spring), "Spring");
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackTest)
