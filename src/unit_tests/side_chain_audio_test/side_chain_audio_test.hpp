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

#ifndef SIDE_CHAIN_AUDIO_TEST_HPP
#define SIDE_CHAIN_AUDIO_TEST_HPP

#include <QObject>

namespace noteahead {

class SideChainAudioTest : public QObject
{
    Q_OBJECT

private slots:
    void test_audioEngine_rebuildProcessingGraph_shouldCorrectlySortIndependentDevices();
    void test_audioEngine_rebuildProcessingGraph_shouldCorrectlySortDependentDevices();
    void test_audioEngine_rebuildProcessingGraph_shouldHandleCircularDependencyGracefully();
    void test_compressorEffect_process_shouldApplySidechainGainReduction();
    void test_compressorEffect_sideChainLpf_bypass_shouldPreserveGainReduction();
    void test_compressorEffect_sideChainLpf_lowCutoff_shouldAttenuateAcDetectorSignal();
    void test_compressorEffect_sideChainLpf_serialization_shouldPreserveValue();
};

} // namespace noteahead

#endif // SIDE_CHAIN_AUDIO_TEST_HPP
