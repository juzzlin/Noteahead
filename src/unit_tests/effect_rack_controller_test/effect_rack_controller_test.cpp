#include "effect_rack_controller_test.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/tracker/parameter_data.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../domain/utility/rta_core.hpp"
#include "../../domain/utility/spectrum_taps.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"
#include "../../plugins/devices/drum_synth_v2/drum_synth_v2_device.hpp"
#include "../../plugins/devices/sampler/sampler_device.hpp"
#include "../../plugins/devices/synth/synth_device.hpp"
#include "../../plugins/effects/air_band_eq/air_band_eq.hpp"
#include "../../plugins/effects/all_pass_filter/all_pass_filter.hpp"
#include "../../plugins/effects/auto_filter/auto_filter.hpp"
#include "../../plugins/effects/auto_panner/auto_panner.hpp"
#include "../../plugins/effects/chorus/chorus.hpp"
#include "../../plugins/effects/clipper/clipper.hpp"
#include "../../plugins/effects/compressor/compressor.hpp"
#include "../../plugins/effects/compressor_76/compressor_76.hpp"
#include "../../plugins/effects/endless_reverb/endless_reverb.hpp"
#include "../../plugins/effects/eq_8_band_parametric/eq_8_band_parametric.hpp"
#include "../../plugins/effects/eq_8_band_parametric/eq_8_band_parametric_presets.hpp"
#include "../../plugins/effects/gain/gain.hpp"
#include "../../plugins/effects/panner/panner.hpp"
#include "../../plugins/effects/phaser/phaser.hpp"
#include "../../plugins/effects/reverb/reverb.hpp"
#include "../../plugins/effects/resonance_tamer/resonance_tamer.hpp"
#include "../../plugins/effects/reverb_224/reverb_224.hpp"
#include "../../plugins/effects/reverb_224/reverb_224_dialog_model.hpp"
#include "../../plugins/effects/room_mics/room_mics.hpp"
#include "../../plugins/effects/saturator/saturator.hpp"
#include "../../plugins/effects/simple_eq/simple_eq.hpp"
#include "../../plugins/effects/tremolo/tremolo.hpp"
#include "../../view/controllers/effect_rack_controller.hpp"
#include "../../view/controllers/send_controller.hpp"
#include "../../view/qml/Dialogs/eq_curve_renderer.hpp"
#include "../../view/qml/Dialogs/rta_renderer.hpp"

#include <QBuffer>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <ranges>

#include "../../domain/devices/device.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"

namespace noteahead {

class MockDevice : public Device
{
public:
    MockDevice(const std::string & name)
      : m_name { name }
    {
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string category() const override
    {
        return "Mock Category";
    }

    std::string typeName() const override
    {
        return "Mock Type";
    }

    std::string typeId() const override
    {
        return "mock";
    }

    void processDeviceMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processDeviceMidiNoteOff(uint8_t) override
    {
    }

    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext &) override
    {
    }

    bool hasActiveAudio() const override
    {
        return false;
    }

    void reset() override
    {
    }

private:
    std::string m_name;
};

namespace {

struct TestContext
{
    std::shared_ptr<AudioEngine> audioEngine;
    std::shared_ptr<DeviceService> deviceService;
    std::shared_ptr<EditorService> editorService;
    EffectRackController controller;
};

TestContext createContext()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    return TestContext {
        audioEngine,
        deviceService,
        editorService,
        EffectRackController { deviceService, editorService }
    };
}

} // namespace

void EffectRackControllerTest::initTestCase()
{
    EffectFactory::init();
}

void EffectRackControllerTest::cleanupTestCase()
{
    EffectFactory::clear();
}

void EffectRackControllerTest::test_effectId_shouldNameTheEffectInThatSlot()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(2, QString::fromStdString(Reverb::typeIdString()));

    QCOMPARE(context.controller.effectId(2), QString::fromStdString(context.deviceService->insertEffectRack().effect(2)->id()));
    QVERIFY(context.controller.effectId(1).isEmpty());
}

void EffectRackControllerTest::test_effectParametersSummary_reverb_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Default reverb: pre-delay 20ms, decay 1500ms
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, decay=1500ms)" });

    // Change parameters
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyPreDelay(), 50.0f / 500.0f); // 50ms
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyDecay(), 3000.0f / 10000.0f); // 3000ms

    const auto summary2 = context.controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(pre=50ms, decay=3000ms)" });
}

void EffectRackControllerTest::test_effectParametersSummary_allPassFilter_shouldReportHertz()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AllPassFilter::typeIdString()));

    // The frequency the filter runs at, not the knob position the project stores: 80 Hz read out
    // as 4036 before, which is where 80 Hz sits on the log knob spread linearly over 20..20000
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(freq=40Hz, stages=1)" });
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyFrequency(), static_cast<float>(ParameterMapper::unmapLogFrequency(80.0, 20.0, 20000.0)));
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(freq=80Hz, stages=1)" });
}

void EffectRackControllerTest::test_effectParametersSummary_endlessReverb_shouldReportPreDelay()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(EndlessReverb::typeIdString()));

    // Defaults: pre-delay 20ms, size 70%, not frozen
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pre=20ms, size=70%, freeze=off)" });

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyPreDelay(), 50.0f / 500.0f); // 50ms

    const auto summary2 = context.controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(pre=50ms, size=70%, freeze=off)" });
}

void EffectRackControllerTest::test_effectParametersSummary_compressor_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Compressor::typeIdString()));

    // Default compressor: detector Peak, attack 0.2 internal -> ~0.5ms, ratio 0.15 internal -> 4:1, sidechain=None
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(Peak, attack=0.5ms, ratio=4:1, sidechain=None)" });

    // Change ratio to 10:1 (internal = (10-1)/19 = 0.473...)
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyRatio(), 9.0f / 19.0f);
    const auto summary2 = context.controller.effectParametersSummary(0);
    QCOMPARE(summary2, QString { "(Peak, attack=0.5ms, ratio=10:1, sidechain=None)" });

    // Put a device in slot 1 and set it as sidechain source
    const auto device1 = std::make_shared<MockDevice>("Device 1");
    context.deviceService->setDevice(1, device1);

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeySideChainSourceDevice(), 1.0f);
    const auto summary3 = context.controller.effectParametersSummary(0);
    QCOMPARE(summary3, QString { "(Peak, attack=0.5ms, ratio=10:1, sidechain=Device 1)" });

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyMode(), 1.0f);
    const auto summary4 = context.controller.effectParametersSummary(0);
    QCOMPARE(summary4, QString { "(RMS, attack=0.5ms, ratio=10:1, sidechain=Device 1)" });
}

void EffectRackControllerTest::test_effectParametersSummary_autoPanner_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoPanner::typeIdString()));

    // Default auto panner: rate 3.52Hz, intensity 100%.
    //
    // It read 1.00Hz until the summary was made to map the rate the way the LFO itself does. The two
    // mappings agree at both ends of the control and are out by three and a half times in the
    // middle, so what the number said and what the pan did were never the same thing.
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(rate=3.52Hz, int=100%)" });

    // The divider is reported in the rate, so a sweep stretched over bars reads as one.
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyRateDivider(), 4.0f);
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(rate=0.88Hz, int=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_autoFilter_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));

    // Default auto filter: low pass at 2.5 kHz swept once a second at a quarter of the range
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(LPF, 2.5kHz, rate=1.00Hz, int=25%)" });

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyFilterType(), 1.0f);
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyLfoMode(), static_cast<float>(Lfo::Mode::BPM));
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyLfoRate(), 0.25f);
    const auto syncedSummary = context.controller.effectParametersSummary(0);
    QCOMPARE(syncedSummary, QString { "(HPF, 2.5kHz, rate=1/4, int=25%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_phaser_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Phaser::typeIdString()));

    // Default phaser: six stages swept once a second, with no feedback
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(6 stages, rate=1.00Hz, fb=0%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_panner_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Panner::typeIdString()));

    // Default panner: pan 50%, width 100%
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(pan=50%, width=100%)" });
}

void EffectRackControllerTest::test_effectParametersSummary_clipper_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Clipper::typeIdString()));

    // Default clipper: threshold 0.0dB
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(thr=0.0dB)" });
}

void EffectRackControllerTest::test_effectParametersSummary_saturator_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Saturator::typeIdString()));

    // Default saturator: drive 6.0dB, mix 100%
    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(drive=6.0dB, mix=100%)" });
}

namespace {

//! Puts @p typeId in slot 0 of @p context.controller's insert rack, which is what every curve here reads.
void setUpInsertRackWith(EffectRackController & controller, const std::string & typeId)
{
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(typeId));
}

} // namespace

void EffectRackControllerTest::test_rtaUpdateRenderer_shouldHandTheAnalyzersLayoutToTheRenderer()
{
    auto context = createContext();
    setUpInsertRackWith(context.controller, Rta::typeIdString());

    RtaRenderer renderer;
    QCOMPARE(renderer.layoutGeneration(), 0u);

    // Invoked by name rather than called, because the dialog reaches this through the meta-object
    // and a direct call proves nothing about that. A parameter whose declaration moc cannot see is
    // recorded by the name as spelled, which QML then fails to resolve against the fully qualified
    // name the class is registered under: the call is dropped at runtime and the analyzer draws
    // nothing at all, with the build and a direct call both perfectly happy.
    QVERIFY(QMetaObject::invokeMethod(&context.controller, "rtaUpdateRenderer", Q_ARG(quint32, 0), Q_ARG(QObject *, &renderer)));

    // The whole path a frame takes: a renderer that never hears about the layout draws every bar in
    // the wrong place.
    QVERIFY(renderer.layoutGeneration() > 0u);

    // A slot with no analyzer in it, something that is not a renderer, and nothing at all are all
    // quiet no-ops.
    context.controller.rtaUpdateRenderer(1, &renderer);
    QObject stranger;
    context.controller.rtaUpdateRenderer(0, &stranger);
    context.controller.rtaUpdateRenderer(0, nullptr);
}

void EffectRackControllerTest::test_spectrumSetActive_postMode_shouldRunOnlyThePostTap()
{
    auto context = createContext();
    setUpInsertRackWith(context.controller, Eq8BandParametric::typeIdString());
    const auto eq = std::dynamic_pointer_cast<Eq8BandParametric>(context.deviceService->insertEffectRack().effect(0));
    QVERIFY(eq);

    EqCurveRenderer renderer;
    renderer.setSpectrumMode(EqCurveRenderer::SpectrumPost);
    // Invoked by name, as the dialog reaches it: see test_rtaUpdateRenderer_shouldHandTheAnalyzersLayoutToTheRenderer
    QVERIFY(QMetaObject::invokeMethod(&context.controller, "spectrumSetActive", Q_ARG(quint32, 0), Q_ARG(QObject *, &renderer), Q_ARG(bool, true)));

    QVERIFY(eq->spectrumTaps().isEnabled());
    QVERIFY(eq->spectrumTaps().core(SpectrumTaps::Tap::Post));
    QVERIFY(!eq->spectrumTaps().core(SpectrumTaps::Tap::Pre));
}

void EffectRackControllerTest::test_spectrumSetActive_inactive_shouldStopTheTaps()
{
    auto context = createContext();
    setUpInsertRackWith(context.controller, Eq8BandParametric::typeIdString());
    const auto eq = std::dynamic_pointer_cast<Eq8BandParametric>(context.deviceService->insertEffectRack().effect(0));

    EqCurveRenderer renderer;
    renderer.setSpectrumMode(EqCurveRenderer::SpectrumPostMinusPre);
    context.controller.spectrumSetActive(0, &renderer, true);
    QVERIFY(eq->spectrumTaps().core(SpectrumTaps::Tap::Pre));
    QVERIFY(eq->spectrumTaps().core(SpectrumTaps::Tap::Post));

    context.controller.spectrumSetActive(0, &renderer, false);
    QVERIFY(!eq->spectrumTaps().isEnabled());
    QVERIFY(eq->isSettled());
}

void EffectRackControllerTest::test_spectrumUpdateRenderer_shouldHandTheLayoutToTheRenderer()
{
    auto context = createContext();
    setUpInsertRackWith(context.controller, Eq8BandParametric::typeIdString());

    EqCurveRenderer renderer;
    renderer.setSpectrumMode(EqCurveRenderer::SpectrumBoth);
    context.controller.spectrumSetActive(0, &renderer, true);
    QCOMPARE(renderer.spectrumLayoutGeneration(), 0u);

    QVERIFY(QMetaObject::invokeMethod(&context.controller, "spectrumUpdateRenderer", Q_ARG(quint32, 0), Q_ARG(QObject *, &renderer)));
    QVERIFY(renderer.spectrumLayoutGeneration() > 0u);

    // A slot with no equalizer in it, something that is not a renderer, and nothing at all are all
    // quiet no-ops
    context.controller.spectrumUpdateRenderer(1, &renderer);
    QObject stranger;
    context.controller.spectrumUpdateRenderer(0, &stranger);
    context.controller.spectrumUpdateRenderer(0, nullptr);
    context.controller.spectrumSetActive(0, nullptr, true);
}

void EffectRackControllerTest::test_eqAutoGainDb_switchedOn_shouldReportTheCompensation()
{
    auto context = createContext();
    setUpInsertRackWith(context.controller, SimpleEq::typeIdString());
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyAmount(), 1.0f);

    QCOMPARE(context.controller.eqAutoGainDb(0), 0.0);
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    QVERIFY(context.controller.eqAutoGainDb(0) < -1.0);

    // A slot with nothing in it has nothing to report
    QCOMPARE(context.controller.eqAutoGainDb(1), 0.0);
}

void EffectRackControllerTest::test_effectParametersSummary_eq8BandParametric_shouldReturnFormattedSummary()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Eq8BandParametric::typeIdString()));

    const auto summary = context.controller.effectParametersSummary(0);
    QCOMPARE(summary, QString { "(Parametric, Mid + Side)" });

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::Mid));
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(Parametric, Mid)" });

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(Eq8BandParametric::StereoMode::Side));
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(Parametric, Side)" });
}

void EffectRackControllerTest::test_effectParametersSummary_emptySlot_shouldReturnEmptyString()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "" });
}

namespace {

//! Drive a rack effect with a stereo sine of the given amplitude, so its meter settles somewhere
//! specific and the summary can be read at a known digit count.
//! The pad character the rack summary lines its numeric fields up with.
const QChar figureSpace { 0x2007 };

void feedSine(const std::shared_ptr<Effect> & effect, double amplitude, double seconds)
{
    static constexpr double sampleRate = 48000.0;
    effect->setSampleRate(sampleRate);
    const auto samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        double l = amplitude * std::sin(2.0 * std::numbers::pi * 1000.0 / sampleRate * i);
        double r = l;
        effect->process(l, r);
    }
}

} // namespace

void EffectRackControllerTest::test_effectParametersSummary_lufsMeter_shouldPadReadingsToConstantWidth()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(LufsMeter::typeIdString()));
    const auto effect = context.deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);

    // A reading crossing -10 gains a digit, and without padding everything after it in the row would
    // shift along. The summary has to keep the same length whatever the meter says.
    const auto atFloor = context.controller.effectParametersSummary(0);
    QCOMPARE(atFloor, QString { "(M=%1-∞ S=%1-∞ I=%1-∞ LUFS)" }.arg(QString { figureSpace }.repeated(3)));

    feedSine(effect, 0.385, 4.0); // ≈ -8 LUFS: one digit before the point, so the field is padded
    const auto oneDigit = context.controller.effectParametersSummary(0);
    QVERIFY(oneDigit.contains(QString { "M=%1-" }.arg(figureSpace)));
    QCOMPARE(oneDigit.length(), atFloor.length());

    feedSine(effect, 0.1, 4.0); // ≈ -21 LUFS: two digits, filling the field
    const auto twoDigits = context.controller.effectParametersSummary(0);
    QVERIFY(twoDigits.contains("M=-2"));
    QCOMPARE(twoDigits.length(), atFloor.length());
}

void EffectRackControllerTest::test_effectParametersSummary_dbtpMeter_shouldPadReadingsToConstantWidth()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(DbTpMeter::typeIdString()));
    const auto effect = context.deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);

    const auto atFloor = context.controller.effectParametersSummary(0);
    QCOMPARE(atFloor, QString { "(L=%1-∞ R=%1-∞ dBTP)" }.arg(QString { figureSpace }.repeated(3)));

    feedSine(effect, 0.35, 0.2); // ≈ -9 dBTP: one digit before the point, so the field is padded
    const auto oneDigit = context.controller.effectParametersSummary(0);
    QVERIFY(oneDigit.contains(QString { "L=%1-" }.arg(figureSpace)));
    QCOMPARE(oneDigit.length(), atFloor.length());

    // A fresh meter, because the peak hold would otherwise keep showing the loud reading.
    context.controller.setEffect(0, QString::fromStdString(DbTpMeter::typeIdString()));
    feedSine(context.deviceService->insertEffectRack().effect(0), 0.035, 0.2); // ≈ -29 dBTP: two digits
    const auto twoDigits = context.controller.effectParametersSummary(0);
    QVERIFY(twoDigits.contains("L=-2"));
    QCOMPARE(twoDigits.length(), atFloor.length());
}

void EffectRackControllerTest::test_isEffectEnabled_shouldReturnEnabledState()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QVERIFY(context.controller.isEffectEnabled(0));

    context.controller.setIsEffectEnabled(0, false);
    QVERIFY(!context.controller.isEffectEnabled(0));
    QVERIFY(context.editorService->isModified());

    context.controller.setIsEffectEnabled(0, true);
    QVERIFY(context.controller.isEffectEnabled(0));
}

void EffectRackControllerTest::test_currentRack_drumVoiceSubIndex_shouldTargetVoiceRack()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    const auto drum = std::make_shared<DrumSynthDevice>("Drum 1");
    deviceService->setDevice(0, drum);

    EffectRackController controller { deviceService, editorService };
    controller.setTargetDeviceName("Drum 1");
    controller.setTargetSubIndex(2);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // The effect lands on voice 2's rack only.
    QVERIFY(drum->voiceEffectRack(2).hasEffects());
    QVERIFY(!drum->voiceEffectRack(0).hasEffects());
    QVERIFY(!drum->insertEffectRack().hasEffects());

    // Switching back to the whole-device rack targets the device insert rack.
    controller.setTargetSubIndex(-1);
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));
    QVERIFY(drum->insertEffectRack().hasEffects());
}

void EffectRackControllerTest::test_copyEffect_sendBus_shouldCopyTheChainToo()
{
    auto context = createContext();

    // The master send rack: no device name, not the insert rack, no sub-index.
    context.controller.setIsInsertRack(false);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // ...and the chain bus 0 runs after its reverb.
    context.controller.setTargetSubIndex(0);
    context.controller.setEffect(0, QString::fromStdString(Phaser::typeIdString()));
    context.controller.setTargetSubIndex(-1);

    context.controller.copyEffect(0, 1);

    // A send bus is its effect plus what the user hung after it, and a copy has to be both.
    const auto copiedEffect = context.deviceService->sendEffectRack().effect(1);
    QVERIFY(copiedEffect);
    QCOMPARE(copiedEffect->typeId(), Reverb::typeIdString());
    const auto copiedChain = context.deviceService->sendChainRack(1).effect(0);
    QVERIFY2(copiedChain, "the chain of the copied bus was left empty");
    QCOMPARE(copiedChain->typeId(), Phaser::typeIdString());
    // The source keeps its own.
    const auto sourceChain = context.deviceService->sendChainRack(0).effect(0);
    QVERIFY(sourceChain);
    QCOMPARE(sourceChain->typeId(), Phaser::typeIdString());
}

void EffectRackControllerTest::test_copyEffect_sendBus_sourceWithNoChain_shouldClearTheTargetChain()
{
    auto context = createContext();

    context.controller.setIsInsertRack(false);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Only the target has a chain of its own.
    context.controller.setTargetSubIndex(1);
    context.controller.setEffect(0, QString::fromStdString(Phaser::typeIdString()));
    context.controller.setTargetSubIndex(-1);

    context.controller.copyEffect(0, 1);

    // The target ends up being what it was copied from, which includes having no chain.
    QVERIFY(!context.deviceService->sendChainRack(1).hasEffects());
}

void EffectRackControllerTest::test_copyEffect_insertRack_shouldLeaveSendChainsAlone()
{
    auto context = createContext();

    // A chain on bus 1 that nothing in this test should touch.
    context.controller.setIsInsertRack(false);
    context.controller.setTargetSubIndex(1);
    context.controller.setEffect(0, QString::fromStdString(Phaser::typeIdString()));
    context.controller.setTargetSubIndex(-1);

    // Copying within a plain list of effects: only the master send rack has buses.
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.copyEffect(0, 1);

    const auto copiedEffect = context.deviceService->insertEffectRack().effect(1);
    QVERIFY(copiedEffect);
    QCOMPARE(copiedEffect->typeId(), Reverb::typeIdString());
    const auto untouchedChain = context.deviceService->sendChainRack(1).effect(0);
    QVERIFY2(untouchedChain, "copying inside an insert rack cleared a send bus chain");
    QCOMPARE(untouchedChain->typeId(), Phaser::typeIdString());
    QVERIFY(!context.deviceService->sendChainRack(0).hasEffects());
}

void EffectRackControllerTest::test_revision_shouldIncrementOnPropertySet()
{
    auto context = createContext();

    QSignalSpy revisionSpy { &context.controller, &EffectRackController::revisionChanged };
    const int initialRevision = context.controller.revision();

    // Setting the same target device name should still increment revision
    context.controller.setTargetDeviceName(context.controller.targetDeviceName());
    QCOMPARE(context.controller.revision(), initialRevision + 1);
    QCOMPARE(revisionSpy.count(), 1);

    // Setting the same isInsertRack value should still increment revision
    context.controller.setIsInsertRack(context.controller.isInsertRack());
    QCOMPARE(context.controller.revision(), initialRevision + 2);
    QCOMPARE(revisionSpy.count(), 2);
}

void EffectRackControllerTest::test_exportSettings_shouldSerializeEffects()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(context.controller.exportSettings(writer));
    buffer.close();

    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains(Reverb::typeIdString().c_str()));
}

void EffectRackControllerTest::test_importSettings_shouldRestoreEffects()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyDecay(), 0.75f);

    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    QVERIFY(context.controller.exportSettings(writer));
    buffer.close();

    context.controller.clearEffect(0);
    QCOMPARE(context.controller.effectType(0), QString {});

    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    QVERIFY(context.controller.importSettings(reader));
    buffer.close();

    QCOMPARE(context.controller.effectType(0), context.controller.reverbType());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyDecay()), 0.75f);
}

void EffectRackControllerTest::test_importEffectSettings_matchingType_shouldEmitConfirmationWithoutMismatch()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Create a temporary reverb settings file
    const QString filePath = "test_matching_effect.nahdeff";
    const auto reverb = std::make_shared<Reverb>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(reverb->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(reverb->type()));
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy spy { &context.controller, &EffectRackController::importEffectSettingsConfirmationRequested };
    context.controller.importEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.at(0);
    QCOMPARE(arguments.at(0).toInt(), 0);
    QCOMPARE(arguments.at(2).toString(), QString::fromStdString(reverb->type()));
    QCOMPARE(arguments.at(3).toString(), QString::fromStdString(reverb->type()));
    QCOMPARE(arguments.at(4).toBool(), false); // No mismatch

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_importEffectSettings_differentType_shouldEmitConfirmationWithMismatch()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    // Create a temporary compressor settings file
    const QString filePath = "test_different_effect.nahdeff";
    const auto compressor = std::make_shared<Compressor>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(compressor->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(compressor->type()));
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy spy { &context.controller, &EffectRackController::importEffectSettingsConfirmationRequested };
    context.controller.importEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(spy.count(), 1);
    const auto arguments = spy.at(0);
    QCOMPARE(arguments.at(4).toBool(), true); // Mismatch

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_confirmImportEffectSettings_shouldImportAndNotify()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    const QString filePath = "test_confirm_effect.nahdeff";
    const auto reverb = std::make_shared<Reverb>();
    QFile file { filePath };
    QVERIFY(file.open(QIODevice::WriteOnly));
    NahdXmlWriter writer { file };
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeStartElement(Constants::NahdXml::xmlKeyEffect());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(reverb->typeId()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyType(), QString::fromStdString(reverb->type()));
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "reverbSize");
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "1230");
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    QSignalSpy revisionSpy { &context.controller, &EffectRackController::revisionChanged };
    context.controller.confirmImportEffectSettings(0, QUrl::fromLocalFile(filePath));

    QCOMPARE(revisionSpy.count(), 1);
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeySize()), 0.123f);
    QVERIFY(context.editorService->isModified());

    QFile::remove(filePath);
}

void EffectRackControllerTest::test_copyEffect_shouldDuplicateAndNotify()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeySize(), 0.42f);

    QSignalSpy revisionSpy { &context.controller, &EffectRackController::revisionChanged };
    context.controller.copyEffect(0, 2);

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(context.editorService->isModified());
    QCOMPARE(context.controller.effectType(2), context.controller.reverbType());
    QCOMPARE(context.controller.parameterValue(2, Constants::NahdXml::xmlKeySize()), 0.42f);
}

void EffectRackControllerTest::test_copyRackFrom_device_shouldReplaceTargetRackAndNotify()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    const auto device = std::make_shared<MockDevice>("Mock Device");
    deviceService->setDevice(0, device);
    EffectRackController controller { deviceService, editorService };

    // Build the source on the device, and something else on the master rack it will replace.
    controller.setIsInsertRack(true);
    controller.setTargetDeviceName("Mock Device");
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));
    controller.setParameterValue(1, Constants::NahdXml::xmlKeySize(), 0.42f);

    controller.setTargetDeviceName({});
    controller.setEffect(0, QString::fromStdString(Compressor::typeIdString()));

    QSignalSpy revisionSpy { &controller, &EffectRackController::revisionChanged };
    QVERIFY(controller.copyRackFrom("Mock Device", true));

    QVERIFY(revisionSpy.count() > 0);
    QVERIFY(editorService->isModified());
    QCOMPARE(controller.effectType(0), QString {});
    QCOMPARE(controller.effectType(1), controller.reverbType());
    QCOMPARE(controller.parameterValue(1, Constants::NahdXml::xmlKeySize()), 0.42f);

    // The copy is independent: editing it does not reach into the rack it came from.
    controller.setParameterValue(1, Constants::NahdXml::xmlKeySize(), 0.1f);
    controller.setTargetDeviceName("Mock Device");
    QCOMPARE(controller.parameterValue(1, Constants::NahdXml::xmlKeySize()), 0.42f);
}

void EffectRackControllerTest::test_copyRackFrom_sameRack_shouldFail()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));

    QVERIFY(!context.controller.copyRackFrom({}, true));
    QCOMPARE(context.controller.effectType(0), context.controller.reverbType());
}

void EffectRackControllerTest::test_availableRackSources_shouldLeaveOutTheTargetRack()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    deviceService->setDevice(0, std::make_shared<MockDevice>("Mock Device"));
    EffectRackController controller { deviceService, editorService };

    const auto names = [](const QVariantList & sources) {
        QStringList list;
        for (const auto & source : sources) {
            list.append(source.toMap()["name"].toString());
        }
        return list;
    };

    // Targeting the master insert rack: the master send rack and the device's rack are left.
    controller.setTargetDeviceName({});
    controller.setIsInsertRack(true);
    auto sources = controller.availableRackSources();
    QCOMPARE(sources.size(), 2);
    QVERIFY(!names(sources).contains("Master Insert Effects"));
    QVERIFY(names(sources).contains("Mock Device"));

    // Targeting the device's rack: both master racks are left.
    controller.setTargetDeviceName("Mock Device");
    sources = controller.availableRackSources();
    QCOMPARE(sources.size(), 2);
    QVERIFY(!names(sources).contains("Mock Device"));

    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setTargetDeviceName({});
    for (const auto & source : controller.availableRackSources()) {
        const auto map = source.toMap();
        QCOMPARE(map["effectCount"].toInt(), map["name"].toString() == "Mock Device" ? 1 : 0);
    }
}

void EffectRackControllerTest::test_revertEffect_shouldRestoreSnapshotAndNotify()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeySize(), 0.42f);

    context.controller.snapshotEffect(0);

    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeySize(), 0.9f);
    context.controller.setIsEffectEnabled(0, false);
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeySize()), 0.9f);

    QSignalSpy revisionSpy { &context.controller, &EffectRackController::revisionChanged };
    QSignalSpy parameterSpy { &context.controller, &EffectRackController::parameterChanged };
    context.controller.revertEffect(0);

    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeySize()), 0.42f);
    QVERIFY(context.controller.isEffectEnabled(0));
    QCOMPARE(revisionSpy.count(), 1);
    // An empty parameter name is how the dialog is told to re-read all of them
    QCOMPARE(parameterSpy.count(), 1);
    QCOMPARE(parameterSpy.at(0).at(1).toString(), QString {});
}

void EffectRackControllerTest::test_revertEffect_withoutSnapshot_shouldKeepEdits()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeySize(), 0.7f);

    context.controller.revertEffect(0);

    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeySize()), 0.7f);
}

void EffectRackControllerTest::test_revertEffect_otherSlot_shouldKeepEdits()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto editorService = std::make_shared<EditorService>();
    EffectRackController controller { deviceService, editorService };

    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    controller.setEffect(1, QString::fromStdString(Reverb::typeIdString()));

    controller.snapshotEffect(0);
    controller.setParameterValue(1, Constants::NahdXml::xmlKeySize(), 0.33f);

    // The snapshot belongs to slot 0, so slot 1 must be left alone
    controller.revertEffect(1);

    QCOMPARE(controller.parameterValue(1, Constants::NahdXml::xmlKeySize()), 0.33f);
}

namespace {

//! A controller on an insert rack with one EQ 8-Band Parametric in slot 0, and a preset store in a
//! directory of the test's own so that nothing is read from or written to the user's real presets.
struct PresetFixture
{
    std::shared_ptr<AudioEngine> audioEngine { std::make_shared<AudioEngine>() };
    std::shared_ptr<DeviceService> deviceService { std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>()) };
    std::shared_ptr<EditorService> editorService { std::make_shared<EditorService>() };
    QTemporaryDir presetRoot;
    std::shared_ptr<PresetService> presetService { std::make_shared<PresetService>(presetRoot.path()) };
    EffectRackController controller { deviceService, editorService };

    explicit PresetFixture(const std::string & typeId = Eq8BandParametric::typeIdString())
    {
        controller.setPresetService(presetService);
        controller.setIsInsertRack(true);
        controller.setEffect(0, QString::fromStdString(typeId));
        controller.snapshotEffect(0);
    }

    int factoryCount() const
    {
        return static_cast<int>(Eq8BandParametricPresets::presets().size());
    }

    //! Position of the named factory preset in the dropdown.
    int factoryIndexOf(const std::string & name) const
    {
        const auto & presets = Eq8BandParametricPresets::presets();
        return static_cast<int>(std::distance(presets.begin(), std::ranges::find_if(presets, [&](const EffectPreset & p) {
                                                  return p.name == name;
                                              })));
    }
};

} // namespace

void EffectRackControllerTest::test_effectPresetNames_eq8_shouldOfferItsFactoryPresets()
{
    PresetFixture f;

    const auto names = f.controller.effectPresetNames(0);
    QCOMPARE(names.size(), f.factoryCount());
    // Numbered by position in this very list, so what the dropdown shows and what loadEffectPreset()
    // is given cannot drift apart.
    QVERIFY2(names.at(0).endsWith("Flat"), qPrintable(names.at(0)));
    QVERIFY(names.at(0).startsWith("000: "));
    for (const auto & name : names) {
        QVERIFY2(!name.endsWith(Constants::userPresetMarker()), qPrintable(name));
    }
}

void EffectRackControllerTest::test_effectPresetNames_effectWithoutPresets_shouldBeEmpty()
{
    // The row hides itself on an empty list, which is how an effect with no patches gets no row.
    PresetFixture f { Phaser::typeIdString() };
    QVERIFY(f.controller.effectPresetNames(0).isEmpty());
}

void EffectRackControllerTest::test_effectPresetNames_emptySlot_shouldBeEmpty()
{
    PresetFixture f;
    QVERIFY(f.controller.effectPresetNames(1).isEmpty());
}

void EffectRackControllerTest::test_loadEffectPreset_shouldApplyItAndNotify()
{
    PresetFixture f;

    QSignalSpy revisionSpy { &f.controller, &EffectRackController::revisionChanged };
    QSignalSpy parameterSpy { &f.controller, &EffectRackController::parameterChanged };
    QSignalSpy presetSpy { &f.controller, &EffectRackController::effectPresetsChanged };

    f.controller.loadEffectPreset(0, f.factoryIndexOf("Lead Vocal"));

    // Band 1 of Lead Vocal is the lower half of its 100 Hz high pass; LowCut is ordinal 4.
    QCOMPARE(f.controller.parameterValue(0, Constants::NahdXml::xmlKeyBandType(0)), 4.0f);
    QCOMPARE(f.controller.currentEffectPresetIndex(0), f.factoryIndexOf("Lead Vocal"));
    QCOMPARE(revisionSpy.count(), 1);
    QCOMPARE(presetSpy.count(), 1);
    // An empty parameter name is how the dialog is told to re-read all of them, which a preset needs
    QCOMPARE(parameterSpy.count(), 1);
    QCOMPARE(parameterSpy.at(0).at(1).toString(), QString {});
}

void EffectRackControllerTest::test_loadEffectPreset_shouldNotBeMarkedAsAUserPreset()
{
    PresetFixture f;
    f.controller.loadEffectPreset(0, f.factoryIndexOf("Snare Drum"));

    QVERIFY(!f.controller.currentEffectPresetIsUserPreset(0));
    QVERIFY(f.controller.currentEffectUserPresetName(0).isEmpty());
    QVERIFY(!f.controller.deleteCurrentEffectUserPreset(0));
}

void EffectRackControllerTest::test_snapshotEffect_shouldSelectTheFirstPreset()
{
    // An effect carries no memory of the preset it was loaded from, so a dialog opening on the name
    // of whatever was picked last time would be showing something untrue.
    PresetFixture f;
    f.controller.loadEffectPreset(0, f.factoryIndexOf("Piano"));
    QVERIFY(f.controller.currentEffectPresetIndex(0) != 0);

    QSignalSpy presetSpy { &f.controller, &EffectRackController::effectPresetsChanged };
    f.controller.snapshotEffect(0);

    QCOMPARE(f.controller.currentEffectPresetIndex(0), 0);
    QCOMPARE(presetSpy.count(), 1);
}

void EffectRackControllerTest::test_saveEffectUserPreset_shouldOfferItAndSelectIt()
{
    PresetFixture f;
    f.controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandGain(0), 0.8f);

    QVERIFY(!f.controller.effectUserPresetExists(0, "My Curve"));
    QVERIFY(f.controller.saveEffectUserPreset(0, "My Curve"));
    QVERIFY(f.controller.effectUserPresetExists(0, "My Curve"));

    const auto names = f.controller.effectPresetNames(0);
    QCOMPARE(names.size(), f.factoryCount() + 1);
    QVERIFY2(names.last().endsWith("My Curve" + Constants::userPresetMarker()), qPrintable(names.last()));

    // The user named this patch, so the dropdown has to agree that this is the patch they are on
    QCOMPARE(f.controller.currentEffectPresetIndex(0), f.factoryCount());
    QVERIFY(f.controller.currentEffectPresetIsUserPreset(0));
    QCOMPARE(f.controller.currentEffectUserPresetName(0), QString { "My Curve" });
}

void EffectRackControllerTest::test_saveEffectUserPreset_effectWithoutFactoryPresets_shouldOfferIt()
{
    // An effect that ships no patches still shows its row on request, so the first preset the user
    // saves there has to become the whole list, and the one selected.
    PresetFixture f { Tremolo::typeIdString() };
    QVERIFY(f.controller.effectPresetNames(0).isEmpty());

    QVERIFY(f.controller.saveEffectUserPreset(0, "My Space"));

    const auto names = f.controller.effectPresetNames(0);
    QCOMPARE(names.size(), 1);
    QVERIFY2(names.first().endsWith("My Space" + Constants::userPresetMarker()), qPrintable(names.first()));
    QCOMPARE(f.controller.currentEffectPresetIndex(0), 0);
    QVERIFY(f.controller.currentEffectPresetIsUserPreset(0));
}

void EffectRackControllerTest::test_saveEffectUserPreset_shouldRoundTripThroughTheStore()
{
    PresetFixture f;
    f.controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandType(0), 1.0f);
    f.controller.setParameterValue(0, Constants::NahdXml::xmlKeyBandGain(0), 0.8f);
    QVERIFY(f.controller.saveEffectUserPreset(0, "My Curve"));

    // Away from the saved patch, then back to it through the store rather than from memory.
    f.controller.loadEffectPreset(0, f.factoryIndexOf("Flat"));
    QCOMPARE(f.controller.parameterValue(0, Constants::NahdXml::xmlKeyBandType(0)), 0.0f);

    f.controller.loadEffectPreset(0, f.factoryCount());

    QCOMPARE(f.controller.parameterValue(0, Constants::NahdXml::xmlKeyBandType(0)), 1.0f);
    QVERIFY(std::abs(f.controller.parameterValue(0, Constants::NahdXml::xmlKeyBandGain(0)) - 0.8f) < 0.001f);
}

void EffectRackControllerTest::test_deleteCurrentEffectUserPreset_shouldRemoveItAndClampTheSelection()
{
    PresetFixture f;
    QVERIFY(f.controller.saveEffectUserPreset(0, "My Curve"));
    QCOMPARE(f.controller.currentEffectPresetIndex(0), f.factoryCount());

    QSignalSpy presetSpy { &f.controller, &EffectRackController::effectPresetsChanged };
    QVERIFY(f.controller.deleteCurrentEffectUserPreset(0));

    QCOMPARE(f.controller.effectPresetNames(0).size(), f.factoryCount());
    QVERIFY(!f.controller.effectUserPresetExists(0, "My Curve"));
    // The list just got shorter under the selection, which must not be left pointing past its end
    QCOMPARE(f.controller.currentEffectPresetIndex(0), f.factoryCount() - 1);
    QCOMPARE(presetSpy.count(), 1);
}

void EffectRackControllerTest::test_deleteCurrentEffectUserPreset_factoryPreset_shouldFail()
{
    PresetFixture f;
    QVERIFY(f.controller.saveEffectUserPreset(0, "My Curve"));
    f.controller.loadEffectPreset(0, f.factoryIndexOf("Flat"));

    QVERIFY(!f.controller.deleteCurrentEffectUserPreset(0));
    QVERIFY(f.controller.effectUserPresetExists(0, "My Curve"));
}

void EffectRackControllerTest::test_effectUserPresets_shouldNotLeakBetweenEffectTypes()
{
    // Presets are keyed by effect type id, so one effect's dropdown must never show a patch that
    // would do nothing if picked.
    PresetFixture f;
    QVERIFY(f.controller.saveEffectUserPreset(0, "My Curve"));

    f.controller.setEffect(1, QString::fromStdString(Phaser::typeIdString()));
    QVERIFY(f.controller.effectPresetNames(1).isEmpty());
    QVERIFY(!f.controller.effectUserPresetExists(1, "My Curve"));
}

void EffectRackControllerTest::test_populatedEffects_shouldReturnOnlyFilledSlots()
{
    auto context = createContext();

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    context.controller.setEffect(2, QString::fromStdString(Compressor::typeIdString()));

    const auto populated = context.controller.populatedEffects();
    QCOMPARE(populated.size(), 2);
    QCOMPARE(populated.at(0).toMap()["slotIndex"].toInt(), 0);
    QCOMPARE(populated.at(1).toMap()["slotIndex"].toInt(), 2);
}

void EffectRackControllerTest::test_availableEffects_shouldBeSortedByName()
{
    auto context = createContext();

    // The gallery is one flat list in this order, and it had silently drifted out of alphabetical
    // as effects were added over time. Sorting is done in availableEffects(); this is what keeps it
    // true for whatever gets added next.
    const auto effects = context.controller.availableEffects();
    QVERIFY(effects.size() > 1);

    for (qsizetype i = 1; i < effects.size(); i++) {
        const auto previous = effects.at(i - 1).toMap()["name"].toString();
        const auto current = effects.at(i).toMap()["name"].toString();
        QVERIFY2(QString::compare(previous, current, Qt::CaseInsensitive) < 0,
                 qPrintable(QString { "\"%1\" is listed before \"%2\"" }.arg(previous, current)));
    }
}

void EffectRackControllerTest::test_availableEffects_shouldHaveCategories()
{
    auto context = createContext();

    // The gallery shows each effect's category after its name
    for (auto && entry : context.controller.availableEffects()) {
        const auto map = entry.toMap();
        QVERIFY2(!map["category"].toString().isEmpty(), qPrintable(map["name"].toString()));
    }
    const auto categoryOf = [&](const QString & name) {
        for (auto && entry : context.controller.availableEffects()) {
            if (const auto map = entry.toMap(); map["name"].toString() == name) {
                return map["category"].toString();
            }
        }
        return QString {};
    };
    QCOMPARE(categoryOf("Phase Scope"), QString { "Meters" });
    QCOMPARE(categoryOf("Reverb"), QString { "Space" });
}

void EffectRackControllerTest::test_partSend_shouldAddressOnePadOnly()
{
    // A pad's send belongs to the pad: setting one must not move its neighbours or the device.
    auto context = createContext();
    SendController sends { context.deviceService };

    DeviceFactory::init();
    const auto device = DeviceFactory::createDevice(DrumSynthDevice::typeIdString(), "Drums");
    context.deviceService->setDevice(0, device);

    QVERIFY(sends.hasPartSends("Drums"));
    sends.setPartSend("Drums", 2, 1, 0.5f);

    QCOMPARE(sends.partSend("Drums", 2, 1), 0.5f);
    QCOMPARE(sends.partSend("Drums", 3, 1), 0.0f);
    QCOMPARE(sends.partSend("Drums", 2, 0), 0.0f);
    QCOMPARE(sends.deviceSend("Drums", 1), 0.0f);
}

void EffectRackControllerTest::test_partSend_negativeSubIndex_shouldBeTheDeviceItself()
{
    // So a dialog that does not know whether it is showing a device or one of its parts can ask the
    // same way for both.
    auto context = createContext();
    SendController sends { context.deviceService };

    DeviceFactory::init();
    const auto device = DeviceFactory::createDevice(SynthDevice::typeIdString(), "Synth");
    context.deviceService->setDevice(0, device);

    sends.setPartSend("Synth", -1, 3, 0.25f);

    QCOMPARE(sends.deviceSend("Synth", 3), 0.25f);
    QCOMPARE(sends.partSend("Synth", -1, 3), 0.25f);
    // A Synth has no parts of its own, so the tab has nothing to show for it.
    QVERIFY(!sends.hasPartSends("Synth"));
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheMonitoringEffects()
{
    auto context = createContext();

    // An effect the factory knows but the gallery does not list is an effect nobody can reach.
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }

    QVERIFY2(types.contains(Constants::RackEffectType::reference()), qPrintable(types.join(", ")));
    QVERIFY2(types.contains(Constants::RackEffectType::crossfeed()), qPrintable(types.join(", ")));
}

void EffectRackControllerTest::test_effectDisplayName_phaseScope_shouldBeTwoWords()
{
    // The gallery and the rack both read the name from the same list
    auto context = createContext();
    QCOMPARE(context.controller.effectDisplayName(context.controller.phaseScopeType()), QString { "Phase Scope" });
}

void EffectRackControllerTest::test_effectDisplayName_registeredEffect_shouldBeGalleryName()
{
    // A rack names its effects by type string, while the gallery lists registered effects by id
    auto context = createContext();
    QCOMPARE(context.controller.effectDisplayName(Constants::RackEffectType::roomMics()), QString { "Room Mics" });
    QCOMPARE(context.controller.effectDisplayName(Constants::RackEffectType::gain()), QString { "Gain" });
    QCOMPARE(context.controller.effectDisplayName(Constants::RackEffectType::stereoWidener()), QString { "Stereo Widener" });
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheAmbience()
{
    auto context = createContext();
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }
    QVERIFY2(types.contains(context.controller.ambienceType()), qPrintable(types.join(", ")));

    // And the factory can make what the gallery offers
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, context.controller.ambienceType());
    QCOMPARE(context.controller.effectType(0), context.controller.ambienceType());
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(size=40%, decay=0.80s, mix=30%)" });
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheCompressor76()
{
    auto context = createContext();
    const auto type = QString::fromStdString(Compressor76::typeIdString());
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }
    QVERIFY2(types.contains(type), qPrintable(types.join(", ")));

    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, type);
    QCOMPARE(context.controller.effectType(0), Constants::RackEffectType::compressor76());
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(4:1, Blackface, in=+0.0dB)" });
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheRoomMics()
{
    auto context = createContext();
    const auto type = QString::fromStdString(RoomMics::typeIdString());
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }
    QVERIFY2(types.contains(type), qPrintable(types.join(", ")));

    // And the factory can make what the gallery offers
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, type);
    QCOMPARE(context.controller.effectType(0), Constants::RackEffectType::roomMics());
    QVERIFY2(context.controller.effectParametersSummary(0).startsWith("(9.0x7.0x3.5 m, RT60 "), qPrintable(context.controller.effectParametersSummary(0)));
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheResonanceTamer()
{
    auto context = createContext();
    const auto type = QString::fromStdString(ResonanceTamer::typeIdString());
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }
    QVERIFY2(types.contains(type), qPrintable(types.join(", ")));

    // And the factory can make what the gallery offers
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, type);
    QCOMPARE(context.controller.effectType(0), Constants::RackEffectType::resonanceTamer());
    QCOMPARE(context.controller.effectDisplayName(type), QString { "Resonance Tamer" });
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(6.0 dB, selectivity=6.0 dB, 100-20.0k Hz)" });
}

void EffectRackControllerTest::test_availableEffects_shouldOfferTheReverb224()
{
    auto context = createContext();
    QStringList types;
    for (auto && entry : context.controller.availableEffects()) {
        types << entry.toMap()["typeId"].toString();
    }
    QVERIFY2(types.contains(context.controller.reverb224Type()), qPrintable(types.join(", ")));

    // And the factory can make what the gallery offers
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, context.controller.reverb224Type());
    QCOMPARE(context.controller.effectType(0), context.controller.reverb224Type());
    QCOMPARE(context.controller.effectParametersSummary(0), QString { "(mid=2.5s, bass=3.0s, pre=20ms, mix=30%)" });
    // Depth has nothing to balance on the plates
    const Reverb224DialogModel model;
    QVERIFY(model.hasEarlyReflections(static_cast<int>(Reverb224::Program::LargeHall)));
    QVERIFY(!model.hasEarlyReflections(static_cast<int>(Reverb224::Program::VocalPlate)));
}

namespace {

//! An automation of the Auto Filter's cutoff, as playback schedules one.
ParameterData cutoffAutomation(const std::shared_ptr<Effect> & effect)
{
    return { 0, 0, effect, Constants::NahdXml::xmlKeyCutoff().toStdString(), ParameterAutomation::Mode::Absolute, 0.5 };
}

} // namespace

void EffectRackControllerTest::test_parameterAutomation_effectInView_shouldRefreshWhenTheChangeLands()
{
    // Automation moves the live value on the audio thread, which cannot tell anybody: the knobs of
    // an open Auto Filter sat still while it played. Playback says so instead, ahead of time, and
    // the dialog is refreshed when the change lands rather than when it was scheduled.
    auto context = createContext();
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));
    const auto effect = context.deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);

    QSignalSpy spy { &context.controller, &EffectRackController::revisionChanged };
    context.deviceService->notifyParameterAutomation(cutoffAutomation(effect), std::chrono::milliseconds { 80 });
    QTest::qWait(30);
    QCOMPARE(spy.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
}

void EffectRackControllerTest::test_parameterAutomation_effectNotInView_shouldNotRefresh()
{
    // The rack in view decides: an automation of anything else costs a comparison and nothing more
    auto context = createContext();
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));
    const std::shared_ptr<Effect> elsewhere = std::make_shared<AutoFilter>();

    QSignalSpy spy { &context.controller, &EffectRackController::revisionChanged };
    context.deviceService->notifyParameterAutomation(cutoffAutomation(elsewhere), std::chrono::milliseconds { 0 });
    QTest::qWait(100);
    QCOMPARE(spy.count(), 0);
}

void EffectRackControllerTest::test_parameterAutomation_burst_shouldRefreshAtMostEvery33ms()
{
    // Every refresh re-reads every binding on the revision, so a busy automation is shown at a
    // screen's pace rather than at its own, and the last change of the burst is still shown
    auto context = createContext();
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));
    const auto effect = context.deviceService->insertEffectRack().effect(0);

    QSignalSpy spy { &context.controller, &EffectRackController::revisionChanged };
    QElapsedTimer clock;
    clock.start();
    // A hundred changes landing over 200 ms
    for (int i = 0; i < 100; i++) {
        context.deviceService->notifyParameterAutomation(cutoffAutomation(effect), std::chrono::milliseconds { i * 2 });
    }
    QTest::qWait(400);
    QVERIFY2(spy.count() >= 2, qPrintable(QString::number(spy.count())));
    QVERIFY2(spy.count() <= static_cast<int>(clock.elapsed() / 33) + 1, qPrintable(QString::number(spy.count())));
    // And nothing further once it is all shown
    const auto settled = spy.count();
    QTest::qWait(100);
    QCOMPARE(spy.count(), settled);
}

void EffectRackControllerTest::test_parameterAutomation_cleared_shouldRefresh()
{
    // Stopping puts every automated value back to what was set, and the knobs have to follow
    auto context = createContext();
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));

    QSignalSpy spy { &context.controller, &EffectRackController::revisionChanged };
    context.deviceService->clearAutomation();
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
}

void EffectRackControllerTest::test_setEffect_masterSendRack_shouldStartMixAtTheTop()
{
    auto context = createContext();

    context.controller.setIsInsertRack(false);
    context.controller.setEffect(0, context.controller.ambienceType());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 1.0f);
    // Only a start: the knob moves as before
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyMix(), 0.4f);
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 0.4f);

    // An insert keeps the effect's own default
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, context.controller.ambienceType());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 0.3f);

    // And so does the chain behind a send bus, which is an insert on the return
    context.controller.setIsInsertRack(false);
    context.controller.setTargetSubIndex(0);
    context.controller.setEffect(0, context.controller.ambienceType());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 0.3f);

    // The Delay keeps its Mix under a key of its own
    context.controller.setTargetSubIndex(-1);
    context.controller.setEffect(0, context.controller.delayType());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyDelayMix()), 1.0f);
}

void EffectRackControllerTest::test_loadEffectPreset_masterSendRack_shouldKeepTheMix()
{
    auto context = createContext();
    const auto mixGlue = [&context] {
        // Shown numbered, as "004: Mix Glue"
        const auto names = context.controller.effectPresetNames(0);
        for (int i = 0; i < names.size(); i++) {
            if (names.at(i).endsWith(": Mix Glue")) {
                return i;
            }
        }
        return -1;
    };

    context.controller.setIsInsertRack(false);
    context.controller.setEffect(0, context.controller.ambienceType());
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyMix(), 0.6f);
    QVERIFY(mixGlue() > 0);
    context.controller.loadEffectPreset(0, mixGlue());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 0.6f);
    // The rest of the preset does land
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeySize()), 0.45f);

    // On an insert the preset's Mix is part of its sound
    context.controller.setIsInsertRack(true);
    context.controller.setEffect(0, context.controller.ambienceType());
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyMix(), 0.6f);
    context.controller.loadEffectPreset(0, mixGlue());
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyMix()), 0.2f);

    // And so does the Delay, whose Mix has a key of its own
    context.controller.setIsInsertRack(false);
    context.controller.setEffect(0, context.controller.delayType());
    context.controller.setParameterValue(0, Constants::NahdXml::xmlKeyDelayMix(), 0.6f);
    QVERIFY(context.controller.effectPresetNames(0).size() > 1);
    context.controller.loadEffectPreset(0, 1);
    QCOMPARE(context.controller.parameterValue(0, Constants::NahdXml::xmlKeyDelayMix()), 0.6f);
}

void EffectRackControllerTest::test_targetSubIndex_devicesWithParts_shouldAddressThePartRack()
{
    // The rack was looked up by device type, and the Drum Synth V2 was missing from the list: its
    // drums' insert dialogs opened on no rack at all, with no slots to add an effect to
    DeviceFactory::init();
    for (auto && typeId : { SamplerDevice::typeIdString(), DrumSynthDevice::typeIdString(), DrumSynthV2Device::typeIdString() }) {
        auto context = createContext();
        const auto device = DeviceFactory::createDevice(typeId, "Parts");
        QVERIFY(device);
        context.deviceService->setDevice(0, device);

        context.controller.setIsInsertRack(true);
        context.controller.setTargetDeviceName("Parts");
        context.controller.setTargetSubIndex(2);
        QVERIFY2(context.controller.effectCount() > 0, typeId.c_str());

        context.controller.setEffect(0, QString::fromStdString(Gain::typeIdString()));
        const auto partRack = device->partEffectRack(2);
        QVERIFY2(partRack && partRack->get().effect(0), typeId.c_str());
        QVERIFY2(!device->insertEffectRack().effect(0), typeId.c_str());
    }
}

void EffectRackControllerTest::test_targetSubIndex_deviceWithoutParts_shouldHaveNoRack()
{
    DeviceFactory::init();
    auto context = createContext();
    const auto device = DeviceFactory::createDevice(SynthDevice::typeIdString(), "Synth");
    context.deviceService->setDevice(0, device);

    context.controller.setIsInsertRack(true);
    context.controller.setTargetDeviceName("Synth");
    context.controller.setTargetSubIndex(2);
    QCOMPARE(context.controller.effectCount(), 0);
    QVERIFY(!device->partEffectRack(2));
}

void EffectRackControllerTest::test_targetSubIndex_masterSendRack_shouldAddressTheBusChain()
{
    // On the master send side a sub-index means a send bus, and points the controller at that bus's
    // chain -- the same shape a Sampler pad or a Drum Synth voice already uses. Without a sub-index
    // it still has to be the send rack itself.
    auto context = createContext();

    context.controller.setTargetDeviceName({});
    context.controller.setIsInsertRack(false);

    context.controller.setTargetSubIndex(-1);
    context.controller.setEffect(0, QString::fromStdString(Reverb::typeIdString()));
    QVERIFY(context.deviceService->sendEffectRack().effect(0));
    QVERIFY(!context.deviceService->sendChainRack(0).hasEffects());

    context.controller.setTargetSubIndex(0);
    context.controller.setEffect(0, QString::fromStdString(Chorus::typeIdString()));
    QVERIFY(context.deviceService->sendChainRack(0).effect(0));
    QCOMPARE(context.deviceService->sendChainRack(0).effect(0)->typeId(), Chorus::typeIdString());
    // The bus's own effect is untouched by an edit to its chain.
    QCOMPARE(context.deviceService->sendEffectRack().effect(0)->typeId(), Reverb::typeIdString());

    // A bus that does not exist addresses nothing rather than the send rack.
    context.controller.setTargetSubIndex(static_cast<int>(context.deviceService->sendChainRackCount()));
    QCOMPARE(context.controller.effectCount(), 0);
}

void EffectRackControllerTest::test_sendChainEffectCount_shouldCountTheChainOfTheGivenBus()
{
    // Drives the count the master rack's send rows show, so it addresses a bus directly rather than
    // through whatever the controller happens to be pointed at.
    auto context = createContext();

    QCOMPARE(context.controller.sendChainEffectCount(0), 0);

    context.deviceService->sendChainRack(1).setEffect(0, EffectFactory::createEffect(Chorus::typeIdString(), {}));
    context.deviceService->sendChainRack(1).setEffect(3, EffectFactory::createEffect(Reverb::typeIdString(), {}));

    QCOMPARE(context.controller.sendChainEffectCount(1), 2);
    QCOMPARE(context.controller.sendChainEffectCount(0), 0);
    QCOMPARE(context.controller.sendChainEffectCount(-1), 0);
    QCOMPARE(context.controller.sendChainEffectCount(static_cast<int>(context.deviceService->sendChainRackCount())), 0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectRackControllerTest)
