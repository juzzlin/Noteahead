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
    QCOMPARE(synth.oscLevel(0), 1.0f);
    QCOMPARE(synth.oscOctave(0), 0);
    QCOMPARE(synth.oscTune(0), 0.0f);
    QCOMPARE(synth.oscWaveform(0), PolyBLEPOscillator::Waveform::Saw);
    QCOMPARE(synth.filterCutoff(), 0.5f);
    QCOMPARE(synth.filterEnvAmount(), 0.0f);
    QCOMPARE(synth.vcaAttack(), 0.1f);
}

void SynthTest::test_oscillatorParameters_shouldSetCorrectValues()
{
    SynthDevice synth;
    synth.setOscLevel(1, 0.5f);
    QCOMPARE(synth.oscLevel(1), 0.5f);
    
    synth.setOscOctave(2, -1);
    QCOMPARE(synth.oscOctave(2), -1);

    synth.setOscTune(0, 10.0f);
    QVERIFY(std::abs(synth.oscTune(0) - 10.0f) < 0.1f);
    
    synth.setOscWaveform(3, PolyBLEPOscillator::Waveform::Pulse);
    QCOMPARE(synth.oscWaveform(3), PolyBLEPOscillator::Waveform::Pulse);
}

void SynthTest::test_adsrParameters_shouldSetCorrectValues()
{
    SynthDevice synth;
    synth.setVcaAttack(0.5f);
    QCOMPARE(synth.vcaAttack(), 0.5f);
    
    synth.setVcfRelease(1.0f);
    QCOMPARE(synth.vcfRelease(), 1.0f);
}

void SynthTest::test_filterParameters_shouldSetCorrectValues()
{
    SynthDevice synth;
    synth.setFilterCutoff(0.8f);
    QCOMPARE(synth.filterCutoff(), 0.8f);
    
    synth.setFilterResonance(0.4f);
    QCOMPARE(synth.filterResonance(), 0.4f);
    
    synth.setFilterKeyTrack(0.5f);
    QCOMPARE(synth.filterKeyTrack(), 0.5f);
}

void SynthTest::test_volume_shouldAdjustMasterVolume()
{
    SynthDevice synth;
    QCOMPARE(synth.volume(), 1.0f);
    synth.setVolume(0.75f);
    QCOMPARE(synth.volume(), 0.75f);
}

void SynthTest::test_velocity_shouldAffectOutputLevel()
{
    SynthDevice synth;
    
    synth.processMidiNoteOn(60, 64); // Half velocity
    
    float outputHalf[2] = {0.0f, 0.0f};
    synth.processAudio(outputHalf, 1, 44100);
    
    synth.processMidiNoteOn(60, 127); // Full velocity
    float outputFull[2] = {0.0f, 0.0f};
    synth.processAudio(outputFull, 1, 44100);
    
    QVERIFY(std::abs(outputFull[0]) > std::abs(outputHalf[0]));
}

void SynthTest::test_detune_shouldAdjustDetuneAmount()
{
    SynthDevice synth;
    QCOMPARE(synth.detune(), 0.0f);
    synth.setDetune(0.5f);
    QCOMPARE(synth.detune(), 0.5f);
}

void SynthTest::test_reset_shouldRestoreDefaults()
{
    SynthDevice synth;
    synth.setOscLevel(0, 0.2f);
    synth.setFilterCutoff(0.1f);
    synth.setFilterEnvAmount(0.8f);
    synth.setVolume(0.5f);
    
    synth.reset();
    
    QCOMPARE(synth.oscLevel(0), 1.0f);
    QCOMPARE(synth.filterCutoff(), 0.5f);
    QCOMPARE(synth.filterEnvAmount(), 0.0f);
    QCOMPARE(synth.volume(), 1.0f);
}

void SynthTest::test_serialization_shouldPreserveValues()
{
    SynthDevice synth1;
    synth1.setOscLevel(0, 0.75f);
    synth1.setOscOctave(0, 1);
    synth1.setVcaAttack(0.2f);
    synth1.setFilterCutoff(0.3f);
    synth1.setVolume(0.45f);

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

    QCOMPARE(synth2.oscLevel(0), 0.75f);
    QCOMPARE(synth2.oscOctave(0), 1);
    QCOMPARE(synth2.vcaAttack(), 0.2f);
    QCOMPARE(synth2.filterCutoff(), 0.3f);
    QCOMPARE(synth2.volume(), 0.45f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::SynthTest)
