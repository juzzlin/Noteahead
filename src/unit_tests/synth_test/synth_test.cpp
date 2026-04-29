#include "synth_test.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../common/constants.hpp"
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QBuffer>

namespace noteahead {

void SynthTest::initTestCase()
{
}

void SynthTest::cleanupTestCase()
{
}

void SynthTest::test_defaultValues_shouldBeCorrect()
{
    SynthDevice synth;
    QCOMPARE(synth.name(), std::string("Notealogue"));
    QCOMPARE(synth.vco1Octave(), 0);
    QCOMPARE(synth.vco1Pitch(), 0.0f);
    QCOMPARE(synth.mixVco1(), 1.0f);
    QCOMPARE(synth.mixVco2(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
    QCOMPARE(synth.hpfCutoff(), 0.0f);
    QCOMPARE(synth.ampSustain(), 1.0f);
}

void SynthTest::test_parameterSetting_shouldUpdateValues()
{
    SynthDevice synth;
    synth.setVco2Waveform(PolyBLEPOscillator::Waveform::Pulse);
    QCOMPARE(synth.vco2Waveform(), PolyBLEPOscillator::Waveform::Pulse);
    
    synth.setMixVco2(0.5f);
    QCOMPARE(synth.mixVco2(), 0.5f);

    synth.setLpfCutoff(0.4f);
    QCOMPARE(synth.lpfCutoff(), 0.4f);
    
    synth.setModInt(0.8f);
    QCOMPARE(synth.modInt(), 0.8f);

    synth.setDelayMix(0.5f);
    QCOMPARE(synth.delayMix(), 0.5f);
    
    synth.setDelaySync(true);
    QCOMPARE(synth.delaySync(), true);
}

void SynthTest::test_polyphony_shouldActiveMultipleVoices()
{
    SynthDevice synth;
    synth.processMidiNoteOn(60, 100);
    synth.processMidiNoteOn(64, 100);
    synth.processMidiNoteOn(67, 100);
    
    // We can't easily check internal voice state without exposing it, 
    // but we can check if audio is generated.
    float output[2048];
    std::fill(output, output + 2048, 0.0f);
    synth.processAudio(output, 1024, 44100);
    
    bool soundDetected = false;
    for (int i = 0; i < 2048; i++) {
        if (std::abs(output[i]) > 0.0001f) {
            soundDetected = true;
            break;
        }
    }
    QVERIFY(soundDetected);
}

void SynthTest::test_presets_shouldLoadCorrectValues()
{
    SynthDevice synth;
    synth.loadPreset(1); // Fat Bass
    
    QCOMPARE(synth.vco1Waveform(), PolyBLEPOscillator::Waveform::Saw);
    QCOMPARE(synth.mixVco2(), 0.8f);
    QCOMPARE(synth.lpfCutoff(), 0.3f);
    QCOMPARE(synth.voiceMode(), SynthDevice::VoiceMode::Unison);
}

void SynthTest::test_midiCc_shouldUpdateParameters()
{
    SynthDevice synth;
    
    // Test individual CC updates
    synth.processMidiCc(7, 64, 0); // Volume ~0.5
    QCOMPARE(synth.masterVolume(), 64.0f / 127.0f);

    synth.processMidiCc(10, 32, 0); // Pan Spread ~0.25
    QCOMPARE(synth.panSpread(), 32.0f / 127.0f);

    synth.processMidiCc(74, 100, 0); // Cutoff ~0.78
    QCOMPARE(synth.lpfCutoff(), 100.0f / 127.0f);

    synth.processMidiCc(81, 10, 0); // HPF Cutoff ~0.08
    QCOMPARE(synth.hpfCutoff(), 10.0f / 127.0f);

    // Test CC 121 (Reset All Controllers)
    // First, set manual UI values
    synth.setMasterVolume(1.0f);
    synth.setPanSpread(0.0f);
    synth.setLpfCutoff(0.5f);
    synth.setHpfCutoff(0.1f);

    // Now change them via MIDI CC
    synth.processMidiCc(7, 10, 0);
    synth.processMidiCc(74, 127, 0);
    
    QCOMPARE(synth.masterVolume(), 10.0f / 127.0f);
    QCOMPARE(synth.lpfCutoff(), 127.0f / 127.0f);

    // Trigger Reset
    synth.processMidiCc(121, 0, 0);

    // Should return to manual UI values
    QCOMPARE(synth.masterVolume(), 1.0f);
    QCOMPARE(synth.panSpread(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 0.5f);
    QCOMPARE(synth.hpfCutoff(), 0.1f);
}

void SynthTest::test_reset_shouldRestoreDefaults()
{
    SynthDevice synth;
    synth.setMixVco2(0.9f);
    synth.setLpfCutoff(0.1f);
    
    synth.reset();
    
    QCOMPARE(synth.mixVco2(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
}

void SynthTest::test_serialization_shouldPreserveValues()
{
    SynthDevice synth1;
    synth1.setVco1Octave(1);
    synth1.setMixVco2(0.75f);
    synth1.setLpfCutoff(0.3f);
    synth1.setAmpAttack(0.2f);

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    synth1.serializeToXml(writer);
    buffer.close();

    SynthDevice synth2;
    QBuffer readBuffer(&data);
    readBuffer.open(QIODevice::ReadOnly);
    QXmlStreamReader reader(&readBuffer);
    
    while (!reader.atEnd() && !reader.isStartElement()) {
        reader.readNext();
    }
    
    synth2.deserializeFromXml(reader);

    QCOMPARE(synth2.vco1Octave(), 1);
    QCOMPARE(synth2.mixVco2(), 0.75f);
    QCOMPARE(synth2.lpfCutoff(), 0.3f);
    QCOMPARE(synth2.ampAttack(), 0.2f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::SynthTest)
