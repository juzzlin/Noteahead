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

#ifndef SONG_OVERVIEW_SERVICE_TEST_HPP
#define SONG_OVERVIEW_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class SongOverviewServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_chain_preInserts_shouldPutTheFaderBeforeTheInserts();
    void test_chain_postInserts_shouldPutTheFaderAfterTheInserts();

    void test_sendTap_postFader_shouldLeaveFromTheEndOfTheChain();
    void test_sendTap_preFader_shouldLeaveFromJustBeforeTheFader();

    void test_subMixerMember_shouldRouteThroughTheGroupAndKeepItsSends();
    void test_ranking_shouldPlaceGroupsRightOfMembersAndMasterLast();
    void test_emptyProject_shouldStillHaveAMaster();

    // The controller is thin translation, but the effect-name lookup in it has a trap worth
    // pinning: the gallery keys some effects by type string and others by id.
    void test_controller_shouldResolveEffectNamesRegisteredByEitherKey();
};

} // namespace noteahead

#endif // SONG_OVERVIEW_SERVICE_TEST_HPP
