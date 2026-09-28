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

#ifndef REFERENCE_ENVIRONMENTS_HPP
#define REFERENCE_ENVIRONMENTS_HPP

#include <array>
#include <string>
#include <vector>

namespace noteahead {

//! What one playback system does to a mix.
//!
//! A model rather than a curve: what makes a car sound like a car is its response, the compression
//! the radio applies, the way the doors ring, and the fact that the two speakers are nowhere near
//! the listener's ears. Every field below is one of those, and an environment that needs none of a
//! given field leaves it at its neutral value.
struct ReferenceEnvironment
{
    //! One resonance or shelf of the system's voicing.
    struct Band
    {
        double frequencyHz { 1000.0 };
        double gainDb { 0.0 };
        double q { 1.0 };
    };

    //! One reflection: how late it arrives, and how much of it comes back.
    struct Tap
    {
        double delayMs { 0.0 };
        double gain { 0.0 };
    };

    //! Name as the preset list shows it.
    std::string name;

    //! Where the system stops reproducing. A phone speaker's job is mostly refusing the bottom two
    //! octaves, so this is the field that carries most environments.
    double highPassHz { 20.0 };
    double lowPassHz { 20000.0 };
    //! Q of those cuts. Above 0.707 the corner peaks, which is what a small sealed box does.
    double highPassQ { 0.707 };
    double lowPassQ { 0.707 };

    //! Up to four bells, applied to both channels.
    std::vector<Band> bands;

    //! The space, if the system is heard in one. Taps are per channel, the right one offset from the
    //! left so the reflections do not arrive as one mono slap.
    std::vector<Tap> taps;
    //! Corner of the damping filter each tap passes through: the later the reflection, the darker.
    double dampingHz { 6000.0 };

    //! What the system's own dynamics do. Ratio 1.0 means it has none worth modelling.
    double thresholdDb { 0.0 };
    double ratio { 1.0 };
    double attackMs { 10.0 };
    double releaseMs { 120.0 };
    //! How hard the system is driven into its own ceiling, 0 for not at all.
    double drive { 0.0 };

    //! How much of the stereo image survives: 1 is untouched, 0 is mono. A phone is one speaker, and
    //! a laptop's two are a few centimetres apart.
    double width { 1.0 };

    //! Level trim, so switching environments compares balances rather than loudnesses.
    double levelDb { 0.0 };
};

using ReferenceEnvironmentList = std::vector<ReferenceEnvironment>;

//! The systems that can be checked against, in the order the environment parameter numbers them.
//! Append only: the number is what a project stores.
const ReferenceEnvironmentList & referenceEnvironments();

} // namespace noteahead

#endif // REFERENCE_ENVIRONMENTS_HPP
