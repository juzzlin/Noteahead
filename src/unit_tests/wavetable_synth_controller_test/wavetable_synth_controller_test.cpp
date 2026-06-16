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

#include "wavetable_synth_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../view/controllers/wavetable_synth_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void WavetableSynthControllerTest::test_properties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int pos = 750;
    controller.setOsc1Pos(pos);
    QCOMPARE(synth->osc1Pos(), static_cast<float>(pos) / Constants::uiInternalScaling());

    const int cutoff = 500;
    controller.setLpfCutoff(cutoff);
    QCOMPARE(synth->lpfCutoff(), static_cast<float>(cutoff) / Constants::uiInternalScaling());
}

void WavetableSynthControllerTest::test_deviceChange_shouldRefreshProperties()
{
    const auto synth1 = std::make_shared<WavetableSynthDevice>("Synth 1");
    const auto synth2 = std::make_shared<WavetableSynthDevice>("Synth 2");
    synth2->setOsc1Pos(0.8f);

    WavetableSynthController controller { synth1 };
    QCOMPARE(controller.osc1Pos(), 0);

    controller.setDevice(synth2);
    QCOMPARE(controller.osc1Pos(), static_cast<int>(std::round(0.8f * Constants::uiInternalScaling())));
}

void WavetableSynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    controller.setOsc1Pos(800);
    QCOMPARE(synth->osc1Pos(), 0.8f);

    synth->reset();
    QCOMPARE(controller.osc1Pos(), 0);
}

void WavetableSynthControllerTest::test_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    QSignalSpy spy { &controller, &WavetableSynthController::osc1PosChanged };
    controller.setOsc1Pos(500);

    // The signal is emitted via WavetableSynthDevice::dataChanged -> WavetableSynthController::requestSettings
    QCOMPARE(spy.count(), 1);
}

void WavetableSynthControllerTest::test_voiceModes()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    const auto modes = controller.voiceModes();
    QCOMPARE(modes.size(), 2);
    QCOMPARE(modes.at(0), QString("Poly"));
    QCOMPARE(modes.at(1), QString("Unison"));
}

void WavetableSynthControllerTest::test_lfo2_properties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int rate = 750;
    controller.setLfo2Rate(rate);
    QCOMPARE(synth->lfo2Rate(), static_cast<float>(rate) / Constants::uiInternalScaling());

    const int intensity = 300;
    controller.setLfo2Int(intensity);
    QCOMPARE(synth->lfo2Int(), static_cast<float>(intensity) / Constants::uiInternalScaling());

    controller.setLfo2Target(static_cast<int>(WavetableSynthDevice::LfoTarget::Cutoff));
    QCOMPARE(synth->lfo2Target(), WavetableSynthDevice::LfoTarget::Cutoff);
}

void WavetableSynthControllerTest::test_lfo2_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    QSignalSpy spy { &controller, &WavetableSynthController::lfo2IntChanged };
    controller.setLfo2Int(500);

    QCOMPARE(spy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableSynthControllerTest)
