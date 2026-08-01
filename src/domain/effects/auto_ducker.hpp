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

#ifndef AUTO_DUCKER_HPP
#define AUTO_DUCKER_HPP

#include "../dsp/cascaded_svf.hpp"
#include "effect.hpp"

#include <cstdint>

namespace noteahead {

//! Level rider driven by a side chain. Whenever the detector rises past the threshold the gain
//! moves towards Amount, which is signed: negative ducks the signal out of the way of the side
//! chain, positive lifts it with the side chain. Without a side chain source the effect listens to
//! its own input.
class AutoDucker : public Effect
{
public:
    AutoDucker();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void reset() override;
    void sync() override;

    std::optional<size_t> sidechainSourceDeviceIndex() const override;

    //! Gain currently applied, in dB. Negative while ducking, positive while boosting.
    float gainDb() const;

private:
    void updateState();
    void updateCoefficients();
    double detectorLevelDb(double left, double right);
    double targetGainDb(double detectorDb) const;
    void updateEnvelope(double targetDb);
    void applyGain(double & left, double & right) const;
    void syncParameters();

    float m_threshold { -20.0f };
    float m_amount { -12.0f };
    float m_knee { 6.0f };
    float m_attackMs { 5.0f };
    float m_releaseMs { 200.0f };
    float m_holdMs { 0.0f };
    float m_sideChainLpfCutoff { 1.0f };
    std::optional<size_t> m_sidechainSourceDevice;

    CascadedSvf m_sideChainLpfL;
    CascadedSvf m_sideChainLpfR;

    double m_attackCoeff { 0.0 };
    double m_releaseCoeff { 0.0 };
    uint32_t m_holdSamples { 0 };
    uint32_t m_holdCounter { 0 };

    double m_envelopeDb { 0.0 };

    bool m_shouldSyncParameters { true };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // AUTO_DUCKER_HPP
