// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "side_chain_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

#include <QSignalSpy>

namespace noteahead {

void SideChainServiceTest::test_setAndGet_sideChainSettings_shouldUpdateModel()
{
    SideChainService model;

    // 1. Set properties on the model and verify getters
    model.setSideChainEnabled(true);
    model.setSideChainSourceTrack(1);
    model.setSideChainSourceColumn(2);
    model.setSideChainLookahead(10);
    model.setSideChainRelease(100);

    QCOMPARE(model.sideChainTargetCount(), 2);

    model.setSideChainTargetEnabled(0, true);
    model.setSideChainTargetController(0, 80);
    model.setSideChainTargetTargetValue(0, 127);
    model.setSideChainTargetReleaseValue(0, 0);

    QCOMPARE(model.sideChainEnabled(), true);
    QCOMPARE(model.sideChainSourceTrack(), quint8 { 1 });
    QCOMPARE(model.sideChainSourceColumn(), quint8 { 2 });
    QCOMPARE(model.sideChainLookahead(), 10);
    QCOMPARE(model.sideChainRelease(), 100);

    QCOMPARE(model.sideChainTargetEnabled(0), true);
    QCOMPARE(model.sideChainTargetController(0), quint8 { 80 });
    QCOMPARE(model.sideChainTargetTargetValue(0), quint8 { 127 });
    QCOMPARE(model.sideChainTargetReleaseValue(0), quint8 { 0 });
}

void SideChainServiceTest::test_renderToEvents_deletedTrack_shouldNotCrash()
{
    Song song;
    const auto automationService = std::make_shared<AutomationService>();
    const auto sideChainService = std::make_shared<SideChainService>();

    // Setup SideChain on Track 1 (index 1), triggered by Track 0 (index 0)
    SideChainSettings sideChainSettings;
    sideChainSettings.enabled = true;
    sideChainSettings.sourceTrackIndex = 0;
    sideChainSettings.sourceColumnIndex = 0;
    sideChainSettings.targets.push_back({ true, 80, 127, 0 });
    sideChainService->setSettings(1, sideChainSettings);

    // Add a note on Track 0 to trigger the side chain
    const Position triggerPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(triggerPosition)->setAsNoteOn(60, 100);

    // Verify it renders fine before deletion
    song.renderToEvents(automationService, sideChainService, 0);

    // Delete Track 1
    song.deleteTrack(1);

    // This should not crash
    song.renderToEvents(automationService, sideChainService, 0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SideChainServiceTest)
