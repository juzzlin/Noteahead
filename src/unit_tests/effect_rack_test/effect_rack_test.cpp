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
#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/volume.hpp"
#include "../../domain/effects/drive.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

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

    auto reverb = std::make_shared<Reverb>();
    rack.setEffect(0, reverb);
    QCOMPARE(rack.effectCount(), Constants::effectRackSize());
    QCOMPARE(rack.effect(0), reverb);

    rack.setEffect(0, nullptr);
    QCOMPARE(rack.effect(0), nullptr);
}

void EffectRackTest::test_version_shouldChangeOnlyWhenEffectsChange()
{
    EffectRack rack;

    // Adding, removing, swapping and clearing effects must bump the version so cached snapshots of
    // effects() (e.g. in AudioEngine::process) get refreshed. Read-only access must not.
    const auto v0 = rack.version();

    rack.setEffect(0, std::make_shared<Volume>());
    const auto v1 = rack.version();
    QVERIFY(v1 != v0);

    // Read-only queries leave the version unchanged.
    (void)rack.effects();
    (void)rack.effect(0);
    (void)rack.effectCount();
    QCOMPARE(rack.version(), v1);

    rack.setEffect(1, std::make_shared<Volume>());
    const auto v2 = rack.version();
    QVERIFY(v2 != v1);

    rack.swapEffects(0, 1);
    const auto v3 = rack.version();
    QVERIFY(v3 != v2);

    rack.removeEffect(0);
    const auto v4 = rack.version();
    QVERIFY(v4 != v3);

    rack.clear();
    QVERIFY(rack.version() != v4);
}

void EffectRackTest::test_rackEnabled_shouldBypassWholeRack()
{
    EffectRack rack;
    QVERIFY(rack.enabled()); // Enabled by default

    auto volume = std::make_shared<Volume>();
    volume->setVolume(0.5f);
    rack.setEffect(0, volume);

    // Disabling the whole rack bypasses it even though the effect itself is enabled.
    rack.setEnabled(false);
    std::vector<double> buffer(4, 1.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, 44100 };
    rack.processInPlace(context);
    for (double sample : buffer) {
        QCOMPARE(sample, 1.0f);
    }

    // Re-enabling applies the effect again.
    rack.setEnabled(true);
    rack.processInPlace(context);
    for (double sample : buffer) {
        QCOMPARE(sample, 0.5f);
    }
}

void EffectRackTest::test_rackEnabled_serialization_shouldRoundTrip()
{
    // A disabled rack round-trips as disabled.
    //
    // Deliberately a registered effect: deserializing goes through EffectFactory, so an effect the
    // factory does not know comes back as nothing and the assertion below would be about the
    // factory rather than about the rack.
    EffectFactory::init();
    EffectRack rack;
    rack.setEffect(0, std::make_shared<Gain>());
    rack.setEnabled(false);

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    rack.serializeEffectsToXml(writer);
    writer.writeEndElement();

    EffectRack loaded;
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement()) {
        if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
            loaded.deserializeEffectsFromXml(reader);
        } else {
            reader.skipCurrentElement();
        }
    }
    QVERIFY(!loaded.enabled());
    QVERIFY(loaded.effect(0) != nullptr);

    // Backward compatibility: an element without the attribute deserializes as enabled.
    const QString legacyXml = "<" + Constants::NahdXml::xmlKeyInsertEffects() + "></" + Constants::NahdXml::xmlKeyInsertEffects() + ">";
    EffectRack legacy;
    legacy.setEnabled(false); // Start disabled to prove the default enables it
    NahdXmlReader legacyReader { legacyXml };
    while (legacyReader.readNextStartElement()) {
        if (legacyReader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
            legacy.deserializeEffectsFromXml(legacyReader);
        } else {
            legacyReader.skipCurrentElement();
        }
    }
    QVERIFY(legacy.enabled());
}

void EffectRackTest::test_setEffect_install_shouldSyncParameters()
{
    // Installing an effect syncs it. Several effects read their parameters only when told to, and
    // not all of them do so in their constructor, so without this a freshly added effect would run
    // on its member defaults until the first knob was moved. The per-sample device paths -- the
    // Drum Synth's per-voice racks -- rely on this, because they never sync during processing.
    const auto gain = std::make_shared<Gain>();
    const auto parameter = gain->parameter(Constants::NahdXml::xmlKeyGain().toStdString());
    QVERIFY(parameter.has_value());
    parameter->get().setValue(1.0f); // Full boost, well away from the unity the member defaults to.

    EffectRack rack;
    rack.setEffect(0, gain);

    std::vector<double> buffer(8, 0.5);
    AudioContext context { std::span(buffer.data(), buffer.size()), 4, 48000 };
    rack.processInPlace(context);

    // Unsynced, the effect would still be at unity and the buffer would come back untouched.
    QVERIFY(buffer[0] > 0.5);
}

void EffectRackTest::test_process_shouldProcessAudio()
{
    EffectRack rack;
    auto reverb = std::make_shared<Reverb>();
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
    auto volume = std::make_shared<Volume>();
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
    auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.75f);
    reverb->setLpfCutoff(0.7f);
    reverb->setHpfCutoff(0.25f);
    rack.setEffect(2, reverb);

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());
    rack.serializeEffectsToXml(writer);
    writer.writeEndElement();

    EffectRack rack2;
    NahdXmlReader reader { xml };
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
    auto reverb2 = std::dynamic_pointer_cast<Reverb>(rack2.effect(2));
    QVERIFY(reverb2 != nullptr);
    QCOMPARE(reverb2->size(), 0.75f);
    QCOMPARE(reverb2->lpfCutoff(), 0.7f);
    QCOMPARE(reverb2->hpfCutoff(), 0.25f);
    QVERIFY(reverb2->enabled());

    // Test disabled serialization
    reverb2->setEnabled(false);
    xml.clear();
    NahdXmlWriter writer2 { xml };
    writer2.writeStartElement(Constants::NahdXml::xmlKeyMasterEffects());
    rack2.serializeEffectsToXml(writer2);
    writer2.writeEndElement();

    EffectRack rack3;
    NahdXmlReader reader2 { xml };
    while (reader2.readNextStartElement()) {
        if (reader2.name() == Constants::NahdXml::xmlKeyMasterEffects()) {
            rack3.deserializeEffectsFromXml(reader2);
        } else {
            reader2.skipCurrentElement();
        }
    }
    auto reverb3 = std::dynamic_pointer_cast<Reverb>(rack3.effect(2));
    QVERIFY(reverb3 != nullptr);
    QVERIFY(!reverb3->enabled());
}

void EffectRackTest::test_enabled_flag_shouldControlProcessing()
{
    EffectRack rack;
    auto volume = std::make_shared<Volume>();
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
    auto reverb = std::make_shared<Reverb>();

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
    auto reverb = std::make_shared<Reverb>();

    const auto presets = Reverb::presetNames();
    QVERIFY(!presets.empty());
    QCOMPARE(presets[0], "Hall");

    reverb->applyPreset(Reverb::stringToPreset("Cathedral"));
    QCOMPARE(reverb->size(), 1.0f);
    QCOMPARE(reverb->lpfCutoff(), 0.84f);
    QCOMPARE(reverb->hpfCutoff(), 0.16f);

    QCOMPARE(Reverb::presetToString(Reverb::Preset::Spring), "Spring");
}

void EffectRackTest::test_exportImportEffectSettings_shouldWorkForSingleEffect()
{
    EffectRack rack;
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.85f);
    rack.setEffect(1, reverb);

    QString xml;
    NahdXmlWriter writer { xml };
    QVERIFY(rack.exportEffectSettings(1, writer));

    // Verify it contains root Settings and Effect
    QVERIFY(xml.contains(Constants::NahdXml::xmlKeySettings()));
    QVERIFY(xml.contains(Constants::NahdXml::xmlKeyEffect()));
    QVERIFY(xml.contains("slot=\"1\""));

    EffectRack rack2;
    NahdXmlReader reader { xml };
    QVERIFY(rack2.importEffectSettings(3, reader)); // Import into slot 3

    const auto reverb2 = std::dynamic_pointer_cast<Reverb>(rack2.effect(3));
    QVERIFY(reverb2 != nullptr);
    QCOMPARE(reverb2->size(), 0.85f);
}

void EffectRackTest::test_importEffectSettings_backwardsCompatibility()
{
    // Old format (whole rack)
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(Reverb::typeIdString()));
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "reverbSize");
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "4200");
    writer.writeEndElement(); // Parameter
    writer.writeEndElement(); // Effect
    writer.writeEndElement(); // InsertEffects
    writer.writeEndElement(); // Settings

    EffectRack rack;
    NahdXmlReader reader { xml };
    QVERIFY(rack.importEffectSettings(0, reader));

    const auto reverb = std::dynamic_pointer_cast<Reverb>(rack.effect(0));
    QVERIFY(reverb != nullptr);
    QCOMPARE(reverb->size(), 0.42f);
}

void EffectRackTest::test_exportImportEffectSettings_drive_shouldRoundTrip()
{
    EffectRack rack;
    const auto drive = std::make_shared<Drive>();
    drive->parameter(Constants::NahdXml::xmlKeyMode().toStdString())->get().update(2.0f); // Fold
    drive->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString())->get().update(0.7f);
    drive->parameter(Constants::NahdXml::xmlKeyMix().toStdString())->get().update(0.4f);
    drive->parameter(Constants::NahdXml::xmlKeyGain().toStdString())->get().update(0.6f);
    rack.setEffect(0, drive);

    QString xml;
    NahdXmlWriter writer { xml };
    QVERIFY(rack.exportEffectSettings(0, writer));

    EffectRack rack2;
    NahdXmlReader reader { xml };
    QVERIFY(rack2.importEffectSettings(1, reader));

    const auto drive2 = std::dynamic_pointer_cast<Drive>(rack2.effect(1));
    QVERIFY(drive2 != nullptr);
    QCOMPARE(drive2->parameter(Constants::NahdXml::xmlKeyMode().toStdString())->get().value(), 2.0f);
    QCOMPARE(drive2->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString())->get().value(), 0.7f);
    QCOMPARE(drive2->parameter(Constants::NahdXml::xmlKeyMix().toStdString())->get().value(), 0.4f);
    QCOMPARE(drive2->parameter(Constants::NahdXml::xmlKeyGain().toStdString())->get().value(), 0.6f);
}

void EffectRackTest::test_copyEffect_shouldDuplicateIntoTargetSlot()
{
    EffectRack rack;
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.85f);
    reverb->setEnabled(false);
    rack.setEffect(1, reverb);

    QVERIFY(!rack.effect(3));
    QVERIFY(rack.copyEffect(1, 3));

    const auto copy = std::dynamic_pointer_cast<Reverb>(rack.effect(3));
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->size(), 0.85f);
    QCOMPARE(copy->enabled(), false);
    // The copy is an independent instance, not the same shared pointer.
    QVERIFY(copy != reverb);
    // The source is left untouched.
    QCOMPARE(rack.effect(1), reverb);
}

void EffectRackTest::test_copyEffect_emptySource_shouldFail()
{
    EffectRack rack;
    rack.setEffect(0, std::make_shared<Reverb>());

    QVERIFY(!rack.copyEffect(2, 0)); // Slot 2 is empty
}

void EffectRackTest::test_copyEffect_sameSlot_shouldFail()
{
    EffectRack rack;
    rack.setEffect(0, std::make_shared<Reverb>());

    QVERIFY(!rack.copyEffect(0, 0));
}

void EffectRackTest::test_copyFrom_shouldCloneEffectsIndependently()
{
    EffectRack source;
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.85f);
    source.setEffect(1, reverb);

    EffectRack target;
    target.setEffect(0, std::make_shared<Reverb>()); // Replaced by the copy
    target.copyFrom(source);

    QVERIFY(!target.effect(0)); // The target's own effect was replaced by the empty source slot
    const auto copy = std::dynamic_pointer_cast<Reverb>(target.effect(1));
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->size(), 0.85f);
    QVERIFY(copy != reverb);

    // The two racks stay independent: the clone can be re-tuned without touching the source.
    copy->setSize(0.25f);
    QCOMPARE(reverb->size(), 0.85f);
}

void EffectRackTest::test_copyFrom_emptySource_shouldClearTarget()
{
    EffectRack source;
    EffectRack target;
    target.setEffect(0, std::make_shared<Reverb>());

    target.copyFrom(source);

    QVERIFY(!target.hasEffects());
    // The rack keeps its fixed slot count so that setEffect() still works on it
    QCOMPARE(target.effectCount(), Constants::effectRackSize());
}

void EffectRackTest::test_swapEffects_shouldSwapTwoSlots()
{
    EffectRack rack;
    const auto reverbA = std::make_shared<Reverb>();
    reverbA->setSize(0.3f);
    const auto reverbB = std::make_shared<Reverb>();
    reverbB->setSize(0.7f);

    rack.setEffect(0, reverbA);
    rack.setEffect(1, reverbB);

    rack.swapEffects(0, 1);

    const auto a = std::dynamic_pointer_cast<Reverb>(rack.effect(0));
    const auto b = std::dynamic_pointer_cast<Reverb>(rack.effect(1));
    QVERIFY(a);
    QVERIFY(b);
    QCOMPARE(a->size(), 0.7f);
    QCOMPARE(b->size(), 0.3f);
}

void EffectRackTest::test_swapEffects_outOfBounds_shouldDoNothing()
{
    EffectRack rack;
    const auto reverb = std::make_shared<Reverb>();
    rack.setEffect(0, reverb);

    rack.swapEffects(0, 999);

    QCOMPARE(rack.effect(0), reverb);
}

void EffectRackTest::test_moveEffect_upwards_shouldShiftSlotsInBetweenDown()
{
    EffectRack rack;
    const auto a = std::make_shared<Reverb>();
    const auto b = std::make_shared<Reverb>();
    const auto c = std::make_shared<Reverb>();
    rack.setEffect(0, a);
    rack.setEffect(1, b);
    rack.setEffect(2, c);

    const auto version = rack.version();
    rack.moveEffect(2, 0); // "Move to top"

    QCOMPARE(rack.effect(0), c);
    QCOMPARE(rack.effect(1), a);
    QCOMPARE(rack.effect(2), b);
    QVERIFY(rack.version() != version);
}

void EffectRackTest::test_moveEffect_downwards_shouldShiftSlotsInBetweenUp()
{
    EffectRack rack;
    const auto a = std::make_shared<Reverb>();
    const auto b = std::make_shared<Reverb>();
    const auto c = std::make_shared<Reverb>();
    rack.setEffect(0, a);
    rack.setEffect(1, b);
    rack.setEffect(2, c);

    const auto lastIndex = rack.effectCount() - 1;
    rack.moveEffect(0, lastIndex); // "Move to bottom"

    QCOMPARE(rack.effect(0), b);
    QCOMPARE(rack.effect(1), c);
    QCOMPARE(rack.effect(2), nullptr);
    QCOMPARE(rack.effect(lastIndex), a);
}

void EffectRackTest::test_moveEffect_sameSlot_shouldDoNothing()
{
    EffectRack rack;
    const auto reverb = std::make_shared<Reverb>();
    rack.setEffect(1, reverb);

    const auto version = rack.version();
    rack.moveEffect(1, 1);

    QCOMPARE(rack.effect(1), reverb);
    QCOMPARE(rack.version(), version);
}

void EffectRackTest::test_moveEffect_outOfBounds_shouldDoNothing()
{
    EffectRack rack;
    const auto reverb = std::make_shared<Reverb>();
    rack.setEffect(0, reverb);

    const auto version = rack.version();
    rack.moveEffect(0, 999);
    rack.moveEffect(999, 0);

    QCOMPARE(rack.effect(0), reverb);
    QCOMPARE(rack.version(), version);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackTest)
