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

#include "magic_master_dialog_model_test.hpp"

#include "../../../../../application/service/device_service.hpp"
#include "../../../../../application/service/editor_service.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/effects/effect_factory.hpp"
#include "../../../../../infra/audio/audio_engine.hpp"
#include "../../../../../infra/data_service.hpp"
#include "../../../../../view/controllers/effect_rack_controller.hpp"
#include "../../magic_master.hpp"
#include "../../magic_master_dialog_model.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace noteahead {

void MagicMasterDialogModelTest::test_telemetryAndCalibrationClear_shouldReflectEffectState()
{
    EffectFactory::init();
    const auto engine { std::make_shared<AudioEngine>() };
    const auto deviceService { std::make_shared<DeviceService>(engine, std::make_shared<DataService>()) };
    const auto editorService { std::make_shared<EditorService>() };
    EffectRackController controller { deviceService, editorService };
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(MagicMaster::typeIdString()));

    MagicMasterDialogModel model;
    model.setRack(&controller);
    model.setEffectIndex(0);

    const auto mm { std::dynamic_pointer_cast<MagicMaster>(deviceService->insertEffectRack().effect(0)) };
    QVERIFY(mm);

    SongContext ctx {};
    ctx.hasSongData = true;
    ctx.bpm = 136.0;
    ctx.lowestBassHz = 38.0;
    ctx.hasSubOr808 = true;
    ctx.summaryText = "136 BPM Test Summary";
    mm->setSongContext(ctx);

    MasterCalibration cal {};
    cal.valid = true;
    cal.driveDb = 3.5;
    cal.calibratedForTargetLufs = -9.0;
    mm->setCalibration(cal);

    mm->setSampleRate(48000.0);
    const size_t frames { 1024 };
    std::vector<double> buf(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s { 0.5 * std::sin(2.0 * std::numbers::pi * 200.0 * static_cast<double>(i) / 48000.0) };
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;
    }
    AudioContext audioCtx { .buffer = buf, .frameCount = static_cast<uint32_t>(frames), .sampleRate = 48000 };
    mm->process(audioCtx);

    QVERIFY(model.isCalibrated());
    QCOMPARE(model.songContextSummary(), QString { "136 BPM Test Summary" });
    QVERIFY(model.truePeakHoldDb() > -10.0f);

    model.clearCalibration();
    QVERIFY(!model.isCalibrated());

    model.resetMeters();
    QVERIFY(model.truePeakHoldDb() <= -69.5f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MagicMasterDialogModelTest)
