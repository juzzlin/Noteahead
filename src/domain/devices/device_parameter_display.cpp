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

#include "device_parameter_display.hpp"

#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../tracker/parameter_container.hpp"
#include "../tracker/parameter_display.hpp"

namespace noteahead::DeviceParameterDisplay {

namespace {

void describe(ParameterContainer & container, const QString & key, ParameterDisplay display)
{
    // setParameterDisplay() ignores a name the container does not have
    container.setParameterDisplay(key.toStdString(), std::move(display));
}

} // namespace

void filters(ParameterContainer & container)
{
    using Display = ParameterDisplay;
    namespace C = Constants::NahdXml;
    describe(container, C::xmlKeyLpfCutoff(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LPF Cutoff") });
    percent(container, C::xmlKeyLpfResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "LPF Resonance"));
    describe(container, C::xmlKeyHpfCutoff(), { .mapping = Display::Mapping::LogFrequencyHighPass, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "HPF Cutoff") });
}

void lfos(ParameterContainer & container)
{
    using Display = ParameterDisplay;
    namespace C = Constants::NahdXml;
    // In hertz even when an LFO is synced to the tempo, where its knob shows a beat division
    // instead: the position is the same either way, only its name changes
    describe(container, C::xmlKeyLfoRate(), { .mapping = Display::Mapping::LfoFrequency, .unit = "Hz", .min = 0.05, .max = 20.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO 1 Rate") });
    describe(container, C::xmlKeyLfo2Rate(), { .mapping = Display::Mapping::LfoFrequency, .unit = "Hz", .min = 0.05, .max = 20.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LFO 2 Rate") });
    bipolar(container, C::xmlKeyLfoIntensity(), QT_TRANSLATE_NOOP("ParameterDisplay", "LFO 1 Intensity"));
    bipolar(container, C::xmlKeyLfo2Intensity(), QT_TRANSLATE_NOOP("ParameterDisplay", "LFO 2 Intensity"));
}

void percent(ParameterContainer & container, const QString & key, const char * label)
{
    describe(container, key, { .unit = "%", .max = 100.0, .label = label });
}

void bipolar(ParameterContainer & container, const QString & key, const char * label)
{
    describe(container, key, { .mapping = ParameterDisplay::Mapping::CubicCentered, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = label });
}

void cents(ParameterContainer & container, const QString & key, const char * label, double range)
{
    describe(container, key, { .mapping = ParameterDisplay::Mapping::CubicCentered, .unit = "c", .min = -range, .max = range, .rest = ParameterDisplay::Rest::Centre, .label = label });
}

} // namespace noteahead::DeviceParameterDisplay
