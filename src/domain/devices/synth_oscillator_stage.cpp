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

#include "synth_oscillator_stage.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! The levels are the device's floats, compared as floats as they always were
bool isAudible(double value)
{
    return static_cast<float>(value) >= 0.001f;
}

} // namespace

std::array<bool, SynthOscillatorStage::VcoCount> SynthOscillatorStage::running(const Settings & settings)
{
    const bool ring = settings.vco2Ring && isAudible(settings.levels.at(1));
    return {
        isAudible(settings.levels.at(0)) || isAudible(settings.crossModDepth) || ring,
        isAudible(settings.levels.at(1)),
        isAudible(settings.levels.at(2)),
        isAudible(settings.levels.at(3)),
    };
}

SynthOscillatorStage::Values SynthOscillatorStage::process(const Oscillators & oscillators, Values frequencies, const std::array<bool, VcoCount> & running, const Settings & settings, double shapeMod, double sampleRate)
{
    Values values {};
    Values oldPhases {};
    for (size_t i = 0; i < VcoCount; i++) {
        auto & oscillator = oscillators.at(i).get();
        if (i == 1 && isAudible(settings.crossModDepth)) {
            // Audio-rate FM: VCO1's instantaneous output modulates VCO2's frequency, mirroring the
            // Minilogue XD's one-way Cross Mod Depth control. VCO3 syncs to the modulated VCO2.
            frequencies.at(1) *= std::exp2(values.at(0) * settings.crossModDepth * 4.0);
        }
        oldPhases.at(i) = oscillator.phase();
        if (!running.at(i)) {
            continue;
        }
        oscillator.setFrequency(frequencies.at(i));
        oscillator.setShape(std::clamp(settings.shapes.at(i) + shapeMod, 0.0, 1.0));
        if (i > 0 && settings.syncs.at(i)) {
            // Hard sync to the oscillator before this one, at the fraction of a sample since it
            // wrapped, so the reset lands between samples where it belongs
            const auto & master = oscillators.at(i - 1).get();
            if (master.phase() < oldPhases.at(i - 1)) {
                const double masterStep = frequencies.at(i - 1) / sampleRate;
                const double step = frequencies.at(i) / sampleRate;
                oscillator.sync(masterStep > 0.0 ? master.phase() / masterStep * step : 0.0);
            }
        }
        oldPhases.at(i) = oscillator.phase();
        values.at(i) = oscillator.nextSample();
        if (i == 1 && settings.vco2Ring) {
            // The Minilogue XD's Ring: VCO2's output is replaced by its product with VCO1. The raw
            // VCO1, like cross mod reads, so VCO1's own filter shapes only what VCO1 adds to the mix.
            values.at(1) *= values.at(0);
        }
    }
    return values;
}

} // namespace noteahead
