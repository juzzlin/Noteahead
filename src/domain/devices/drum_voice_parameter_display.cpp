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

#include "drum_voice_parameter_display.hpp"

#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../tracker/parameter_container.hpp"
#include "../tracker/parameter_display.hpp"

namespace noteahead {

void describeDrumVoiceParameters(ParameterContainer & container, const std::string & prefix)
{
    using Display = ParameterDisplay;
    namespace C = Constants::NahdXml;
    const auto describe = [&](const QString & key, Display display) {
        // setParameterDisplay() ignores a name the container does not have
        container.setParameterDisplay(prefix + key.toStdString(), std::move(display));
    };
    const auto percent = [&](const QString & key, const char * label) {
        describe(key, { .unit = "%", .max = 100.0, .label = label });
    };

    percent(C::xmlKeyLevel(), QT_TRANSLATE_NOOP("ParameterDisplay", "Level"));
    describe(C::xmlKeyPan(), { .mapping = Display::Mapping::Pan, .unit = "%", .min = -100.0, .max = 100.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Pan") });
    // LowPassFilter and HighPassFilter, whose travel starts at 0 Hz
    describe(C::xmlKeyCutoff(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = 0.0, .max = 20000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "LPF Cutoff") });
    describe(C::xmlKeyHpfCutoff(), { .mapping = Display::Mapping::LogFrequencyHighPass, .unit = "Hz", .min = 0.0, .max = 20000.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "HPF Cutoff") });

    // The engine's own controls, which the dialogs show as positions too
    percent(C::xmlKeyTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tune"));
    percent(C::xmlKeyDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Decay"));
    percent(C::xmlKeyAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Attack"));
    percent(C::xmlKeyClickTune(), QT_TRANSLATE_NOOP("ParameterDisplay", "Click Tune"));
    percent(C::xmlKeyPitchDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth"));
    percent(C::xmlKeyPitchDecay(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Decay"));
    percent(C::xmlKeySnappy(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snappy"));
    percent(C::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"));
    percent(C::xmlKeyResonance(), QT_TRANSLATE_NOOP("ParameterDisplay", "Resonance"));

    // The amp envelope V2 lays over the engine, in the dialog's ranges
    describe(C::xmlKeyAmpAttack(), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = 1.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Attack") });
    describe(C::xmlKeyAmpHold(), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = 8.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Hold") });
    describe(C::xmlKeyAmpDecay(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = 0.005, .max = 8.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Decay") });
    percent(C::xmlKeyAmpSustain(), QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Sustain"));
    describe(C::xmlKeyAmpRelease(), { .mapping = Display::Mapping::Exponential, .unit = "s", .min = 0.005, .max = 8.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Amp Release") });
}

} // namespace noteahead
