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

import QtQuick 2.15
import Noteahead 1.0

Knob {
    id: knobRoot
    property var controller: null
    //! A high pass is out of the way at the bottom of its travel, a low pass at the top, and that is
    //! the end the readout calls Bypass.
    property bool isHpf: false
    //! For the LowPassFilter and HighPassFilter the Drum Synths and the Sampler run, which put the
    //! bottom of the travel at 0 Hz. Every other device's filter starts at 20 Hz.
    property bool startsAtZero: false

    // The curve the filters themselves follow: equal octaves from 20 Hz, or the 0 Hz curve, both up
    // to 20 kHz or just short of Nyquist when that is lower. See CascadedSvf and Utils::Dsp::cutoffToHz.
    mapping: isHpf ? "logFrequencyHighPass" : "logFrequency"
    mapMin: startsAtZero ? 0 : 20
    mapMax: controller ? Math.min(20000, controller.sampleRate * 0.49) : 20000
}
