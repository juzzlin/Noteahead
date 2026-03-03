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

#ifndef CLAP_808_ENGINE_HPP
#define CLAP_808_ENGINE_HPP

#include "drum_808_parts.hpp"
#include "drum_engine.hpp"

namespace noteahead {

//! A TR-808 hand clap or maracas, modelled on recordings of an RD-8 and on a sweep of its Offset.
//!
//! The clap is noise in a band around 1.6 kHz, struck three times 9 ms apart and then let ring as a
//! tail of a few tens of milliseconds. Offset is how loud and how long all of it is: on the RD-8 the
//! clap is 19 dB down at zero. The maracas are a burst of high noise held for 20 ms and cut. The
//! channel's switch picks which, as on the RD-8.
class Clap808Engine : public DrumEngine
{
public:
    Clap808Engine();
    ~Clap808Engine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    //! The clap's length, 0..1 with the RD-8's at noon
    void setOffset(float offset);
    //! The channel's switch: maracas instead of a clap
    void setMaracas(bool maracas);

private:
    double clapSample(double t);
    double maracasSample(double t);

    float m_offset { 0.5f };
    bool m_maracas { false };

    bool m_active { false };
    double m_time { 0.0 };
    double m_velocity { 1.0 };
    double m_level { 1.0 };
    double m_burstScale { 1.0 };
    double m_tailLevel { 1.0 };
    double m_tailScale { 1.0 };
    Drum808::Noise m_burstNoise { 0xc1a9u };
    Drum808::Noise m_tailNoise { 0x7a11u };
    Drum808::Biquad m_burstBand {};
    Drum808::Biquad m_burstLowPass {};
    Drum808::Biquad m_tailBand {};
    Drum808::Biquad m_tailLowPass {};
    Drum808::Biquad m_highPass1 {};
    Drum808::Biquad m_highPass2 {};
    Drum808::Biquad m_lowPass {};
};

} // namespace noteahead

#endif // CLAP_808_ENGINE_HPP
