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

#ifndef CHANNEL_STRIP_TEST_HPP
#define CHANNEL_STRIP_TEST_HPP

#include <QObject>

namespace noteahead {

class ChannelStripTest : public QObject
{
    Q_OBJECT

private slots:
    void test_defaults_shouldMatchLegacyChain();
    void test_applyFader_shouldScaleTheWholeBuffer();
    void test_applyFader_unityShouldLeaveBufferUntouched();
    void test_faderPosition_nonLinearInsert_shouldDifferBetweenOrderings();
    void test_faderPosition_linearInsert_shouldAgreeBetweenOrderings();
    void test_faderPosition_postInserts_shouldNotChangeInsertInput();
    void test_sendTap_preFader_shouldIgnoreFader();
    void test_sendTap_postFader_shouldFollowFader();
    void test_settings_shouldRoundTripThroughXml();
    void test_settings_absentFromXml_shouldLoadAsLegacy();
};

} // namespace noteahead

#endif // CHANNEL_STRIP_TEST_HPP
