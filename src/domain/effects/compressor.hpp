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

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/compressor_core.hpp"
#include "effect.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace noteahead {

class Compressor : public Effect
{
public:
    using DetectorMode = CompressorCore::DetectorMode;

    //! Starting points calibrated against Noteahead's own signal levels rather than ported from
    //! elsewhere: its devices run quiet and span some 20 dB between them, so thresholds taken from
    //! general practice either do nothing or clamp down hard. Each is set against the material it
    //! is named for. See applyPreset().
    enum class Preset
    {
        Glue,
        DrumBus,
        PunchyDrums,
        Piano,
        Vocal,
        Bass,
        Pump,
        Brickwall
    };

    Compressor();

    //! Writes the preset over this compressor's parameters. Routing -- the sidechain source -- is
    //! left alone: it is the user's patching, not part of the sound the preset describes.
    void applyPreset(Preset preset);

    static std::string presetToString(Preset preset);
    static Preset stringToPreset(const std::string & presetName);
    static std::vector<std::string> presetNames();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;

    float reductionDb() const;

private:
    void updateBuffers();
    void applyGain(double & left, double & right);
    void syncParameters();

    CompressorCore m_core;

    float m_makeup { 0.0f };
    float m_lookaheadMs { 0.0f };
    float m_sideChainLpfCutoff { 1.0f };
    std::optional<size_t> m_sidechainSourceDevice;

    CascadedSvf m_sideChainLpfL;
    CascadedSvf m_sideChainLpfR;

    std::vector<double> m_delayBufferL;
    std::vector<double> m_delayBufferR;
    uint32_t m_writePos { 0 };
    uint32_t m_delaySamples { 0 };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // COMPRESSOR_HPP
