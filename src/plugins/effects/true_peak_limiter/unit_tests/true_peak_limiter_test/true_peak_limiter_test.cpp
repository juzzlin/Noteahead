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

#include "true_peak_limiter_test.hpp"

#include "../../../../../application/service/device_service.hpp"
#include "../../../../../application/service/editor_service.hpp"
#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/effects/effect_factory.hpp"
#include "../../../../../domain/utility/loudness_analyzer.hpp"
#include "../../../../../infra/audio/audio_engine.hpp"
#include "../../../../../infra/data_service.hpp"
#include "../../../../../view/controllers/effect_rack_controller.hpp"
#include "../../true_peak_limiter.hpp"
#include "../../true_peak_limiter_dialog_model.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double Rate { 48000.0 };

} // namespace

void TruePeakLimiterTest::test_typeAndMetadata_shouldMatchSpecification()
{
    TruePeakLimiter effect;
    QCOMPARE(QString::fromStdString(effect.type()), Constants::RackEffectType::truePeakLimiter());
    QCOMPARE(QString::fromStdString(effect.typeId()), QString::fromStdString(TruePeakLimiter::typeIdString()));
    QCOMPARE(effect.modulationPriority().size(), size_t { 3 });
}

void TruePeakLimiterTest::test_latencyFrames_beforeProcessing_shouldReflectLookaheadParameter()
{
    TruePeakLimiter effect;
    QCOMPARE(effect.latencyFrames(48000.0), size_t { 245 });

    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
        p->get().setValue(1.0f); // 10 ms -> 480 samples -> 6 + 479 = 485
    }
    QCOMPARE(effect.latencyFrames(48000.0), size_t { 485 });

    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
        p->get().setValue(0.0f); // 0 ms -> 1 sample -> 6 frames
    }
    QCOMPARE(effect.latencyFrames(48000.0), size_t { 6 });
}

void TruePeakLimiterTest::test_factoryPresets_shouldApplyCleanly()
{
    TruePeakLimiter effect;
    const auto & presets { effect.factoryPresets() };
    QVERIFY(presets.size() >= 6);
    QCOMPARE(QString::fromStdString(presets.front().name), QString { "Init" });

    for (size_t i = 0; i < presets.size(); i++) {
        QVERIFY(effect.applyFactoryPreset(i));
        effect.sync();
    }
    QVERIFY(!effect.applyFactoryPreset(presets.size()));
}

void TruePeakLimiterTest::test_hotInterSampleSignal_shouldNeverExceedCeiling()
{
    TruePeakLimiter effect;
    effect.setSampleRate(Rate);
    effect.setBpm(140.0f);
    QVERIFY(effect.applyFactoryPreset(2)); // EDM Master Loud (thresh=-12 dB, ceil=-1.2 dBTP, boost=true)
    effect.sync();

    const size_t frames { 24000 };
    std::vector<double> buffer(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s { 0.9 * std::sin(2.0 * std::numbers::pi * 0.25 * static_cast<double>(i) + std::numbers::pi * 0.25) };
        buffer[i * 2] = s;
        buffer[i * 2 + 1] = -s;
    }

    AudioContext ctx {
        .buffer = buffer,
        .frameCount = static_cast<uint32_t>(frames),
        .sampleRate = static_cast<uint32_t>(Rate),
    };
    effect.process(ctx);

    LoudnessAnalyzer analyzer { Rate };
    std::vector<float> floats(frames * 2, 0.0f);
    for (size_t i = 0; i < frames * 2; i++) {
        floats[i] = static_cast<float>(buffer[i]);
    }
    analyzer.process(floats.data(), floats.size());
    const auto result { analyzer.calculate() };

    QVERIFY2(result.truePeak <= -1.2f + 0.02f, qPrintable(QString { "Analyzer truePeak=%1" }.arg(result.truePeak)));
    QVERIFY2(effect.outputTruePeakHoldDb() <= -1.2f + 0.02f, qPrintable(QString { "Effect hold=%1" }.arg(effect.outputTruePeakHoldDb())));
    QVERIFY(effect.reductionDb() < -3.0f);
}

void TruePeakLimiterTest::test_reset_shouldBeDeterministic()
{
    TruePeakLimiter effect;
    effect.setSampleRate(Rate);
    QVERIFY(effect.applyFactoryPreset(2));
    effect.sync();

    const size_t frames { 2048 };
    std::vector<double> a(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s { 0.7 * std::sin(2.0 * std::numbers::pi * 120.0 * static_cast<double>(i) / Rate) };
        a[i * 2] = s;
        a[i * 2 + 1] = s;
    }
    auto b { a };

    AudioContext ctxA { .buffer = a, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    effect.process(ctxA);
    effect.reset();

    AudioContext ctxB { .buffer = b, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    effect.process(ctxB);

    for (size_t i = 0; i < frames * 2; i++) {
        QCOMPARE(b[i], a[i]);
    }
}

void TruePeakLimiterTest::test_dialogModel_shouldExposeMetersAndResetPeakHold()
{
    EffectFactory::init();
    const auto engine { std::make_shared<AudioEngine>() };
    const auto deviceService { std::make_shared<DeviceService>(engine, std::make_shared<DataService>()) };
    const auto editorService { std::make_shared<EditorService>() };
    EffectRackController controller { deviceService, editorService };
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(TruePeakLimiter::typeIdString()));

    TruePeakLimiterDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);

    const auto effect { deviceService->insertEffectRack().effect(0) };
    QVERIFY(effect);
    effect->setSampleRate(Rate);
    std::vector<double> buf(512 * 2, 0.9);
    AudioContext ctx { .buffer = buf, .frameCount = 512, .sampleRate = static_cast<uint32_t>(Rate) };
    effect->process(ctx);

    QVERIFY(model.outputTruePeakHoldDb() > -10.0f);
    QVERIFY(model.outputTruePeakDb() > -70.0f);

    model.resetPeakHold();
    QVERIFY(model.outputTruePeakHoldDb() <= -69.5f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TruePeakLimiterTest)
