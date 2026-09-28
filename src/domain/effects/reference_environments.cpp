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

#include "reference_environments.hpp"

namespace noteahead {

const ReferenceEnvironmentList & referenceEnvironments()
{
    // Indicative rather than measured: each one is the handful of things that system does which
    // change a mix decision, and nothing else. The numbers are round on purpose -- they are meant to
    // be read and argued with, not to claim a precision no parametric model has.
    static const ReferenceEnvironmentList environments {
        // A car is a small sealed box with the speakers in the doors, behind the listener's knees.
        // Its bottom is a lift around the cabin's own mode rather than real extension, the midrange
        // is scooped by the glass and the seats, and the radio compresses everything so it survives
        // the road noise.
        ReferenceEnvironment {
          .name = "Car",
          .highPassHz = 55.0,
          .lowPassHz = 15000.0,
          .highPassQ = 1.2,
          .bands = { { 70.0, 5.0, 1.0 }, { 320.0, 3.0, 1.2 }, { 900.0, -3.5, 1.0 }, { 4000.0, 2.5, 0.9 } },
          .taps = { { 7.0, 0.22 }, { 11.0, 0.18 }, { 17.0, 0.12 } },
          .dampingHz = 4000.0,
          .thresholdDb = -22.0,
          .ratio = 3.5,
          .attackMs = 15.0,
          .releaseMs = 180.0,
          .width = 0.7,
          .levelDb = -1.0 },

        // A club is a big PA in a reverberant room: enormous sub, a deliberate dip where the mix
        // would otherwise be shouty, and limiting on the way in. Anything muddy below 100 Hz turns
        // into the loudest thing in the building.
        ReferenceEnvironment {
          .name = "Club",
          .highPassHz = 32.0,
          .lowPassHz = 17000.0,
          .bands = { { 45.0, 6.0, 0.8 }, { 180.0, -2.0, 1.0 }, { 700.0, -3.0, 0.9 }, { 3000.0, 2.0, 0.8 } },
          .taps = { { 23.0, 0.3 }, { 37.0, 0.25 }, { 59.0, 0.2 }, { 83.0, 0.15 } },
          .dampingHz = 3500.0,
          .thresholdDb = -18.0,
          .ratio = 6.0,
          .attackMs = 5.0,
          .releaseMs = 250.0,
          .drive = 0.3,
          .width = 0.85,
          .levelDb = -2.0 },

        // One tiny driver held at arm's length. Nothing below 500 Hz exists at all, the midrange is
        // peaky, and it is driven into distortion at any volume worth hearing. This is the one that
        // decides whether the vocal and the snare carry.
        ReferenceEnvironment {
          .name = "Phone",
          .highPassHz = 520.0,
          .lowPassHz = 8000.0,
          .highPassQ = 1.4,
          .bands = { { 900.0, 2.5, 1.2 }, { 1800.0, 4.0, 1.5 }, { 3400.0, 3.0, 1.2 }, { 6000.0, -4.0, 1.0 } },
          .thresholdDb = -20.0,
          .ratio = 4.0,
          .attackMs = 3.0,
          .releaseMs = 90.0,
          .drive = 0.55,
          .width = 0.0,
          .levelDb = -1.0 },

        // Two small drivers a few centimetres apart, firing down at the desk. Some bottom, a boxy
        // lower midrange, and an image barely wider than the machine.
        ReferenceEnvironment {
          .name = "Laptop",
          .highPassHz = 220.0,
          .lowPassHz = 14000.0,
          .highPassQ = 1.1,
          .bands = { { 300.0, 3.0, 1.2 }, { 800.0, -2.5, 1.0 }, { 2500.0, 3.0, 1.0 }, { 5000.0, -2.0, 1.0 } },
          .taps = { { 4.0, 0.15 }, { 9.0, 0.1 } },
          .dampingHz = 5000.0,
          .thresholdDb = -24.0,
          .ratio = 2.5,
          .drive = 0.2,
          .width = 0.25,
          .levelDb = -1.0 },

        // A large reverberant space heard from the back of it: the tail is most of what arrives, the
        // top is eaten by the air and the top is long gone by the time it reaches the listener.
        ReferenceEnvironment {
          .name = "Hall",
          .highPassHz = 40.0,
          .lowPassHz = 11000.0,
          .bands = { { 120.0, 2.0, 0.8 }, { 2000.0, -2.5, 0.7 }, { 6000.0, -4.0, 0.8 } },
          .taps = { { 31.0, 0.32 }, { 47.0, 0.28 }, { 71.0, 0.24 }, { 97.0, 0.2 }, { 131.0, 0.16 }, { 173.0, 0.12 } },
          .dampingHz = 2500.0,
          .width = 1.0,
          .levelDb = -2.0 },

        // A cheap portable stereo: a plastic box with a mid-bass bump where its port is, no real sub,
        // and a lift up top that is meant to read as detail.
        ReferenceEnvironment {
          .name = "Boombox",
          .highPassHz = 90.0,
          .lowPassHz = 13000.0,
          .highPassQ = 1.3,
          .bands = { { 130.0, 5.0, 1.4 }, { 400.0, -3.0, 1.0 }, { 3000.0, 2.0, 1.0 }, { 8000.0, 3.0, 0.9 } },
          .taps = { { 5.0, 0.2 }, { 13.0, 0.14 } },
          .dampingHz = 4500.0,
          .thresholdDb = -20.0,
          .ratio = 3.0,
          .drive = 0.35,
          .width = 0.5,
          .levelDb = -1.0 },

        // In-ears: more bottom than they have any right to, a scooped midrange and a sharpened top,
        // which is the consumer tuning nearly all of them ship with.
        ReferenceEnvironment {
          .name = "Earbuds",
          .highPassHz = 45.0,
          .lowPassHz = 18000.0,
          .bands = { { 80.0, 6.5, 0.8 }, { 500.0, -3.0, 1.0 }, { 3000.0, 4.5, 1.2 }, { 9000.0, 3.0, 1.0 } },
          .width = 1.0,
          .levelDb = -2.0 },

        // A flat panel's downward-firing drivers, plus the loudness processing every set applies:
        // dialogue pulled forward, everything else held down behind it.
        ReferenceEnvironment {
          .name = "TV",
          .highPassHz = 150.0,
          .lowPassHz = 12000.0,
          .highPassQ = 1.0,
          .bands = { { 250.0, -2.0, 1.0 }, { 1500.0, 3.5, 1.0 }, { 4000.0, 2.0, 1.0 }, { 7000.0, -3.0, 1.0 } },
          .taps = { { 6.0, 0.18 }, { 14.0, 0.12 } },
          .dampingHz = 4000.0,
          .thresholdDb = -26.0,
          .ratio = 4.5,
          .attackMs = 20.0,
          .releaseMs = 300.0,
          .width = 0.4,
          .levelDb = -1.0 },

        // The honest one: a small sealed nearfield of the kind mixes have been checked on for
        // decades. No sub, no top, a forward midrange, and nothing else to hide behind.
        ReferenceEnvironment {
          .name = "Nearfield",
          .highPassHz = 65.0,
          .lowPassHz = 16000.0,
          .highPassQ = 0.9,
          .bands = { { 200.0, -1.5, 0.9 }, { 1500.0, 2.0, 0.8 }, { 5000.0, 1.5, 0.9 } },
          .taps = { { 3.0, 0.12 } },
          .dampingHz = 7000.0,
          .width = 0.95,
          .levelDb = 0.0 }
    };
    return environments;
}

} // namespace noteahead
