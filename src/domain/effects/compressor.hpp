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
#include "effect.hpp"

#include <cstdint>
#include <vector>

namespace noteahead {

class Compressor : public Effect
{
public:
    enum class DetectorMode
    {
        Peak,
        Rms
    };

    Compressor();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;

    float reductionDb() const;

private:
    void updateBuffers();
    void updateCoefficients();
    double calculateDetectorLevelDb(double left, double right);
    double calculateGainReductionDb(double detectorDb) const;
    void updateEnvelope(double gainReductionDb);
    void applyGain(double & left, double & right);
    void syncParameters();

    float m_threshold { -20.0f };
    float m_ratio { 4.0f };
    float m_attackMs { 10.0f };
    float m_releaseMs { 100.0f };
    float m_knee { 0.0f };
    float m_makeup { 0.0f };
    float m_lookaheadMs { 0.0f };
    float m_sideChainLpfCutoff { 1.0f };
    DetectorMode m_detectorMode { DetectorMode::Peak };
    std::optional<size_t> m_sidechainSourceDevice;

    CascadedSvf m_sideChainLpfL;
    CascadedSvf m_sideChainLpfR;

    double m_attackCoeff { 0.0 };
    double m_releaseCoeff { 0.0 };
    double m_rmsCoeff { 0.0 };

    double m_rmsSquare { 0.0 };

    double m_envelopeDb { 0.0 };
    double m_reductionDb { 0.0 };

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
