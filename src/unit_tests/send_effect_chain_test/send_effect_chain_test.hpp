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

#ifndef SEND_EFFECT_CHAIN_TEST_HPP
#define SEND_EFFECT_CHAIN_TEST_HPP

#include <QObject>

namespace noteahead {

class SendEffectChainTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sendChain_empty_shouldMatchTheSendEffectAlone();
    void test_sendChain_withEffect_shouldShapeWhatTheSendReturns();
    void test_sendChain_withoutSendEffect_shouldNotDoubleTheDry();
    void test_sendChain_withDisabledSendEffect_shouldTakeOverTheSendRole();
    void test_sendChain_disabledRack_shouldMatchTheSendEffectAlone();
    void test_sendChain_disabledEffect_shouldBeSkipped();
    void test_sendChain_addedAfterProcess_shouldBeApplied();
    void test_sendChain_sendRackBypassed_shouldStopToo();
    void test_sendChain_ordering_shouldRunInSlotOrder();
};

} // namespace noteahead

#endif // SEND_EFFECT_CHAIN_TEST_HPP
