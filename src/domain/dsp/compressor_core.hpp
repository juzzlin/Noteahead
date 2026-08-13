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

#ifndef COMPRESSOR_CORE_HPP
#define COMPRESSOR_CORE_HPP

#include "dsp_component.hpp"

namespace noteahead {

//! The detector, gain computer and ballistics of a feed-forward compressor, with no audio path of
//! its own: it is handed a detector frame and answers with the gain to apply, in dB.
//!
//! Split out from the audio path so that a multiband strip can run one per band off a detector that
//! is not the signal being compressed, which is what side chaining and band splitting both need.
class CompressorCore : public DspComponent
{
public:
    enum class DetectorMode
    {
        Peak,
        Rms
    };

    void setThresholdDb(double thresholdDb);
    void setRatio(double ratio);
    void setKneeDb(double kneeDb);
    void setAttackMs(double attackMs);
    void setReleaseMs(double releaseMs);
    void setDetectorMode(DetectorMode mode);

    //! Advances the detector and the envelope by one frame and returns the gain to apply, in dB.
    //! Never positive: a compressor only ever pulls level down.
    double processGainDb(double left, double right);

    //! Gain currently applied, in dB, for metering. Zero when nothing is being held down.
    double reductionDb() const;

    void reset();

private:
    void updateCoefficients();
    double detectorLevelDb(double left, double right);
    double gainComputerDb(double detectorDb) const;

    double m_thresholdDb { -20.0 };
    double m_ratio { 4.0 };
    double m_kneeDb { 0.0 };
    double m_attackMs { 10.0 };
    double m_releaseMs { 100.0 };
    DetectorMode m_detectorMode { DetectorMode::Peak };

    double m_attackCoeff { 0.0 };
    double m_releaseCoeff { 0.0 };
    double m_rmsCoeff { 0.0 };

    double m_rmsSquare { 0.0 };
    double m_envelopeDb { 0.0 };

    double m_lastAttackMs { -1.0 };
    double m_lastReleaseMs { -1.0 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // COMPRESSOR_CORE_HPP
