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

#include "drum_synth_808_device.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/drum/clap_808_engine.hpp"
#include "../../../domain/dsp/drum/drum_808_parts.hpp"
#include "../../../domain/dsp/drum/kick_808_engine.hpp"
#include "../../../domain/dsp/drum/metal_808_engine.hpp"
#include "../../../domain/dsp/drum/rim_808_engine.hpp"
#include "../../../domain/dsp/drum/snare_808_engine.hpp"
#include "../../../domain/dsp/drum/tom_808_engine.hpp"
#include "../../../domain/tracker/parameter_display.hpp"

#include <QtGlobal>

#include <array>
#include <cmath>
#include <functional>
#include <numbers>

namespace noteahead {

using namespace DrumSynthV2;
namespace NahdXml = Constants::NahdXml;

namespace {

//! What a knob does to the drum it belongs to
using Setter = std::function<void(DrumEngine &, float)>;

struct PadKnob
{
    QString key;
    const char * label;
    Setter set;
    //! Where it rests: where the RD-8's panel marks it, which is noon unless said otherwise
    float defaultValue = 0.5f;
    bool isSwitch = false;
};

template<typename Engine>
Setter setter(void (Engine::*method)(float))
{
    return [method](DrumEngine & engine, float value) { (static_cast<Engine &>(engine).*method)(value); };
}

template<typename Engine>
Setter switcher(void (Engine::*method)(bool))
{
    return [method](DrumEngine & engine, float value) { (static_cast<Engine &>(engine).*method)(value >= 0.5f); };
}

//! The RD-8's kick Tuning, in semitones from where it is at noon: barely a third of an octave
//! up from there and two thirds down, the panel's default marked at 3. Measured on a sweep.
constexpr Drum808::KnobCurve KickTuning { { 0.0, 0.3, 0.5, 0.8, 1.0 }, { -8.326, -3.702, 0.0, 3.041, 4.857 } };
//! Where the RD-8's kick is at noon against C2, where Kick808Engine's Tuning is at noon
constexpr double KickNoonSemitones = -0.183;

//! The kick pad's Tuning, onto Kick808Engine's, whose two octaves either way are Kick 808's own
void setKickTuning(DrumEngine & engine, float tuning)
{
    const double semitones = KickTuning.at(tuning) + KickNoonSemitones;
    static_cast<Kick808Engine &>(engine).setTune(static_cast<float>(0.5 + semitones / (2.0 * Kick808Engine::TuneRangeSemitones)));
}

//! Where the RD-8's panel marks the default of the clap's Offset and of the hats' Tone: a little
//! past noon
constexpr float PanelMark = 0.52f;

//! Every pad's knobs, the one list the parameters, the syncing and the dialog are all made from
std::vector<PadKnob> padKnobs(int index)
{
    const auto tuning = QT_TRANSLATE_NOOP("ParameterDisplay", "Tuning");
    const auto tone = QT_TRANSLATE_NOOP("ParameterDisplay", "Tone");
    const auto decay = QT_TRANSLATE_NOOP("ParameterDisplay", "Decay");
    switch (static_cast<VoiceIndex>(index)) {
    case VoiceIndex::Kick:
        // Kick 808's kick on the RD-8's Tuning, which the panel marks at 3, and with its Decay at
        // noon, which rings as long as the RD-8's at noon. Tone rests where a new Kick 808's does.
        return {
            { NahdXml::xmlKeyTune(), tuning, setKickTuning, 0.3f },
            { NahdXml::xmlKeyTone(), tone, setter(&Kick808Engine::setTone), 0.35f },
            { NahdXml::xmlKeyDecay(), decay, setter(&Kick808Engine::setDecay), 0.5f },
        };
    case VoiceIndex::Snare:
        return {
            { NahdXml::xmlKeyTone(), tone, setter(&Snare808Engine::setTone) },
            { NahdXml::xmlKeySnappy(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snappy"), setter(&Snare808Engine::setSnappy) },
        };
    case VoiceIndex::LowTom:
    case VoiceIndex::MidTom:
    case VoiceIndex::HighTom:
        return {
            { NahdXml::xmlKeyTune(), tuning, setter(&Tom808Engine::setTune) },
            { NahdXml::xmlKeyVoiceSwitch(), "Conga", switcher(&Tom808Engine::setConga), 0.0f, true },
        };
    case VoiceIndex::Rim:
        return { { NahdXml::xmlKeyVoiceSwitch(), "Claves", switcher(&Rim808Engine::setClaves), 0.0f, true } };
    case VoiceIndex::Clap:
        return {
            { NahdXml::xmlKeyOffset(), QT_TRANSLATE_NOOP("ParameterDisplay", "Offset"), setter(&Clap808Engine::setOffset), PanelMark },
            { NahdXml::xmlKeyVoiceSwitch(), "Maracas", switcher(&Clap808Engine::setMaracas), 0.0f, true },
        };
    case VoiceIndex::ClosedHiHat:
        return { { NahdXml::xmlKeyTone(), tone, setter(&Metal808Engine::setTone), PanelMark } };
    case VoiceIndex::OpenHiHat:
        return {
            { NahdXml::xmlKeyTone(), tone, setter(&Metal808Engine::setTone), PanelMark },
            { NahdXml::xmlKeyDecay(), decay, setter(&Metal808Engine::setDecay) },
        };
    case VoiceIndex::Crash:
    case VoiceIndex::ReverseCrash:
        return {
            { NahdXml::xmlKeyTone(), tone, setter(&Metal808Engine::setTone) },
            { NahdXml::xmlKeyDecay(), decay, setter(&Metal808Engine::setDecay) },
        };
    case VoiceIndex::Ride:
        break;
    }
    return {};
}

std::unique_ptr<DrumEngine> makeEngine(int index)
{
    switch (static_cast<VoiceIndex>(index)) {
    case VoiceIndex::Kick:
        return std::make_unique<Kick808Engine>();
    case VoiceIndex::Snare:
        return std::make_unique<Snare808Engine>();
    case VoiceIndex::ClosedHiHat:
        return std::make_unique<Metal808Engine>(Metal808Engine::Sound::ClosedHat);
    case VoiceIndex::OpenHiHat:
        return std::make_unique<Metal808Engine>(Metal808Engine::Sound::OpenHat);
    case VoiceIndex::Clap:
        return std::make_unique<Clap808Engine>();
    case VoiceIndex::LowTom:
        return std::make_unique<Tom808Engine>(Tom808Engine::Position::Low);
    case VoiceIndex::MidTom:
        return std::make_unique<Tom808Engine>(Tom808Engine::Position::Mid);
    case VoiceIndex::HighTom:
        return std::make_unique<Tom808Engine>(Tom808Engine::Position::High);
    case VoiceIndex::Crash:
        return std::make_unique<Metal808Engine>(Metal808Engine::Sound::Cymbal);
    case VoiceIndex::Ride:
        return std::make_unique<Metal808Engine>(Metal808Engine::Sound::Cowbell);
    case VoiceIndex::ReverseCrash:
        return std::make_unique<Metal808Engine>(Metal808Engine::Sound::Cymbal, true);
    case VoiceIndex::Rim:
        break;
    }
    return std::make_unique<Rim808Engine>();
}

//! Where each pad's peak sits against the kick's in the default mix, in dB: Drum Synth 909's, the
//! cowbell on the ride's pad louder than a ride, as an 808's cowbell is played
constexpr std::array<double, NumVoices> MixTargetDb { 0.0, -2.5, -8.0, -3.0, -7.0, -4.0, -4.0, -4.0, -9.0, -8.0, -10.0, -6.0 };

//! How much a pad panned to @p pan peaks over one in the centre: the pan law is constant power,
//! so a pad's louder side is at its strongest at either end
double panPeakOverCentre(double pan)
{
    const double angle = pan * std::numbers::pi * 0.5;
    return std::max(std::cos(angle), std::sin(angle)) / std::cos(std::numbers::pi * 0.25);
}

} // namespace

DrumSynth808Device::DrumSynth808Device(std::string name)
  : DrumPadDevice { std::move(name) }
{
    for (int i { 0 }; i < NumVoices; i++) {
        setVoiceEngine(i, makeEngine(i));
    }
    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
        for (auto && knob : padKnobs(i)) {
            if (!knob.isSwitch) {
                setParameterDisplay(voiceId(i) + "_" + knob.key.toStdString(), { .unit = "", .max = 10.0, .label = knob.label });
            }
        }
    }
    DrumSynth808Device::syncParameters();
}

std::string DrumSynth808Device::typeName() const
{
    return Constants::drumSynth808DeviceName().toStdString();
}

std::string DrumSynth808Device::typeIdString()
{
    return "7a3c5e18-80b0-4d6f-9c21-5e8d0f6b2a47";
}

std::string DrumSynth808Device::typeId() const
{
    return typeIdString();
}

std::unique_ptr<DrumPadDevice> DrumSynth808Device::newInstance(std::string name) const
{
    return std::make_unique<DrumSynth808Device>(std::move(name));
}

std::string DrumSynth808Device::voiceName(int index) const
{
    switch (static_cast<VoiceIndex>(index)) {
    case VoiceIndex::Crash:
        return "Cymbal";
    case VoiceIndex::Ride:
        return "Cowbell";
    case VoiceIndex::ReverseCrash:
        return "Reverse Cymbal";
    default:
        break;
    }
    return DrumPadDevice::voiceName(index);
}

std::vector<std::string> DrumSynth808Device::kitNames()
{
    return { "Init", "Mono", "Equal", "Mono Equal" };
}

void DrumSynth808Device::loadKit(Kit kit)
{
    const bool mono = kit == Kit::Mono || kit == Kit::MonoEqual;
    const bool equal = kit == Kit::Equal || kit == Kit::MonoEqual;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        // A kit is the whole device, so everything it does not set goes back to its default first
        reset();
        for (int pad = 0; pad < NumVoices; pad++) {
            const auto [fader, pan] = defaultVoiceMix(pad);
            double gain = ParameterMapper::mapFader(static_cast<double>(fader));
            if (equal) {
                gain *= std::pow(10.0, -MixTargetDb.at(static_cast<size_t>(pad)) / 20.0);
            }
            if (mono) {
                // Centred, a pad loses what its louder side had, so the fader makes it up
                gain *= panPeakOverCentre(static_cast<double>(pan));
            }
            const std::string prefix { voiceId(pad) + "_" };
            if (auto p = parameter(prefix + NahdXml::xmlKeyFader().toStdString()); p) {
                p->get().setValue(static_cast<float>(ParameterMapper::unmapFader(gain)));
            }
            if (auto p = parameter(prefix + NahdXml::xmlKeyPan().toStdString()); p) {
                p->get().setValue(mono ? 0.5f : pan);
            }
        }
        syncParameters();
    }
    emit dataChanged();
}

std::vector<DrumSynth808Device::Knob> DrumSynth808Device::voiceKnobs(int index)
{
    std::vector<Knob> knobs;
    for (auto && knob : padKnobs(index)) {
        knobs.push_back({ knob.key.toStdString(), knob.label, knob.isSwitch });
    }
    return knobs;
}

void DrumSynth808Device::addEngineParameters(int index, const std::string & prefix)
{
    for (auto && knob : padKnobs(index)) {
        if (knob.isSwitch) {
            addParameter(Parameter { prefix + knob.key.toStdString(), knob.defaultValue, 0, 1, static_cast<int>(knob.defaultValue), 1, Parameter::Type::Boolean });
        } else {
            addParameter(Parameter { prefix + knob.key.toStdString(), knob.defaultValue, 0, 10000, static_cast<int>(std::lround(knob.defaultValue * 10000)), 100 });
        }
    }
}

void DrumSynth808Device::syncEngineParameters(int index, const std::string & prefix)
{
    auto & engine = voiceEngine(index);
    for (auto && knob : padKnobs(index)) {
        if (const auto p = parameter(prefix + knob.key.toStdString()); p) {
            knob.set(engine, p->get().value());
        }
    }
}

std::pair<float, float> DrumSynth808Device::defaultVoiceMix(int index) const
{
    // Fader and pan as knob positions. The faders are set from each drum's measured peak, through
    // the device's soft clip, to MixTargetDb; the pans are Drum Synth V2's.
    static constexpr std::array<std::pair<float, float>, NumVoices> mix { {
      { 0.7500f, 0.500f }, // Kick
      { 0.2541f, 0.300f }, // Snare
      { 0.2664f, 0.600f }, // ClosedHiHat
      { 0.6237f, 0.500f }, // Clap
      { 0.1083f, 0.350f }, // OpenHiHat
      { 0.5201f, 0.675f }, // LowTom
      { 0.4814f, 0.500f }, // MidTom
      { 0.1998f, 0.325f }, // HighTom
      { 0.1096f, 0.775f }, // Crash: the cymbal
      { 0.2173f, 0.700f }, // Ride: the cowbell
      { 0.0901f, 0.775f }, // ReverseCrash: the cymbal reversed
      { 0.1791f, 0.300f }, // Rim
    } };
    return mix.at(static_cast<size_t>(index));
}

std::vector<std::string> DrumSynth808Device::modulationPriority() const
{
    return {
        NahdXml::xmlKeyCutoff().toStdString(),
        NahdXml::xmlKeyAmpDecay().toStdString(),
        NahdXml::xmlKeyTone().toStdString(),
        NahdXml::xmlKeyDecay().toStdString(),
        NahdXml::xmlKeyTune().toStdString(),
        NahdXml::xmlKeySnappy().toStdString(),
        NahdXml::xmlKeyHpfCutoff().toStdString(),
    };
}

} // namespace noteahead
