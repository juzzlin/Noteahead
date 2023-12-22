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

#include "drum_synth_909_device.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/drum/clap_909_engine.hpp"
#include "../../../domain/dsp/drum/kick_909_engine.hpp"
#include "../../../domain/dsp/drum/metal_909_engine.hpp"
#include "../../../domain/dsp/drum/rim_909_engine.hpp"
#include "../../../domain/dsp/drum/snare_909_engine.hpp"
#include "../../../domain/dsp/drum/tom_909_engine.hpp"
#include "../../../domain/tracker/parameter_display.hpp"

#include <QtGlobal>

#include <array>
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
};

template<typename Engine>
Setter setter(void (Engine::*method)(float))
{
    return [method](DrumEngine & engine, float value) { (static_cast<Engine &>(engine).*method)(value); };
}

//! Every pad's knobs, the one list the parameters, the syncing and the dialog are all made from
std::vector<PadKnob> padKnobs(int index)
{
    const auto tune = QT_TRANSLATE_NOOP("ParameterDisplay", "Tune");
    const auto decay = QT_TRANSLATE_NOOP("ParameterDisplay", "Decay");
    switch (static_cast<VoiceIndex>(index)) {
    case VoiceIndex::Kick:
        return {
            { NahdXml::xmlKeyTune(), tune, setter(&Kick909Engine::setTune) },
            { NahdXml::xmlKeyAttack(), QT_TRANSLATE_NOOP("ParameterDisplay", "Attack"), setter(&Kick909Engine::setAttack) },
            { NahdXml::xmlKeyDecay(), decay, setter(&Kick909Engine::setDecay) },
            { NahdXml::xmlKeyPitch(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch"), setter(&Kick909Engine::setPitch) },
            { NahdXml::xmlKeyPitchDepth(), QT_TRANSLATE_NOOP("ParameterDisplay", "Pitch Depth"), setter(&Kick909Engine::setPitchDepth) },
        };
    case VoiceIndex::Snare:
        return {
            { NahdXml::xmlKeyTune(), tune, setter(&Snare909Engine::setTune) },
            { NahdXml::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"), setter(&Snare909Engine::setTone) },
            { NahdXml::xmlKeySnappy(), QT_TRANSLATE_NOOP("ParameterDisplay", "Snappy"), setter(&Snare909Engine::setSnappy) },
        };
    case VoiceIndex::LowTom:
    case VoiceIndex::MidTom:
    case VoiceIndex::HighTom:
        return {
            { NahdXml::xmlKeyTune(), tune, setter(&Tom909Engine::setTune) },
            { NahdXml::xmlKeyDecay(), decay, setter(&Tom909Engine::setDecay) },
        };
    case VoiceIndex::ClosedHiHat:
    case VoiceIndex::OpenHiHat:
        return {
            { NahdXml::xmlKeyTune(), tune, setter(&Metal909Engine::setTune) },
            { NahdXml::xmlKeyDecay(), decay, setter(&Metal909Engine::setDecay) },
        };
    case VoiceIndex::Crash:
    case VoiceIndex::ReverseCrash:
    case VoiceIndex::Ride:
        return { { NahdXml::xmlKeyTune(), tune, setter(&Metal909Engine::setTune) } };
    case VoiceIndex::Clap:
    case VoiceIndex::Rim:
        break;
    }
    return {};
}

std::unique_ptr<DrumEngine> makeEngine(int index)
{
    switch (static_cast<VoiceIndex>(index)) {
    case VoiceIndex::Kick:
        return std::make_unique<Kick909Engine>();
    case VoiceIndex::Snare:
        return std::make_unique<Snare909Engine>();
    case VoiceIndex::ClosedHiHat:
        return std::make_unique<Metal909Engine>(Metal909Engine::Sound::ClosedHat);
    case VoiceIndex::OpenHiHat:
        return std::make_unique<Metal909Engine>(Metal909Engine::Sound::OpenHat);
    case VoiceIndex::Clap:
        return std::make_unique<Clap909Engine>();
    case VoiceIndex::LowTom:
        return std::make_unique<Tom909Engine>(Tom909Engine::Tom::Low);
    case VoiceIndex::MidTom:
        return std::make_unique<Tom909Engine>(Tom909Engine::Tom::Mid);
    case VoiceIndex::HighTom:
        return std::make_unique<Tom909Engine>(Tom909Engine::Tom::High);
    case VoiceIndex::Crash:
        return std::make_unique<Metal909Engine>(Metal909Engine::Sound::Crash);
    case VoiceIndex::Ride:
        return std::make_unique<Metal909Engine>(Metal909Engine::Sound::Ride);
    case VoiceIndex::ReverseCrash:
        return std::make_unique<Metal909Engine>(Metal909Engine::Sound::Crash, true);
    case VoiceIndex::Rim:
        break;
    }
    return std::make_unique<Rim909Engine>();
}

//! The RD-9's knobs all rest at noon
constexpr float KnobDefault { 0.5f };

//! Where each pad's peak sits against the kick's in the default mix, in dB
constexpr std::array<double, NumVoices> MixTargetDb { 0.0, -2.5, -8.0, -3.0, -7.0, -4.0, -4.0, -4.0, -9.0, -10.0, -10.0, -6.0 };

//! What a pad needs on top of its distance from the kick to peak where the kick does. The device
//! soft-clips its output, which takes the kick's peak 1.5 dB down and a quieter pad's less, so the
//! distances measured after it are short of what the faders must make up. Measured.
constexpr std::array<double, NumVoices> EqualTrimDb { 0.0, 0.87, 1.65, 1.09, 1.39, 1.26, 1.25, 1.24, 1.94, 1.60, 1.81, 1.40 };

//! How much a pad panned to @p pan peaks over one in the centre: the pan law is constant power,
//! so a pad's louder side is at its strongest at either end
double panPeakOverCentre(double pan)
{
    const double angle = pan * std::numbers::pi * 0.5;
    return std::max(std::cos(angle), std::sin(angle)) / std::cos(std::numbers::pi * 0.25);
}

} // namespace

DrumSynth909Device::DrumSynth909Device(std::string name)
  : DrumPadDevice { std::move(name) }
{
    for (int i { 0 }; i < NumVoices; i++) {
        setVoiceEngine(i, makeEngine(i));
    }
    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
        for (auto && knob : padKnobs(i)) {
            setParameterDisplay(voiceId(i) + "_" + knob.key.toStdString(), { .unit = "", .max = 10.0, .label = knob.label });
        }
    }
    DrumSynth909Device::syncParameters();
}

std::string DrumSynth909Device::typeName() const
{
    return Constants::drumSynth909DeviceName().toStdString();
}

std::string DrumSynth909Device::typeIdString()
{
    return "9addce72-166b-46df-9eca-7eaeed08a4d3";
}

std::string DrumSynth909Device::typeId() const
{
    return typeIdString();
}

std::unique_ptr<DrumPadDevice> DrumSynth909Device::newInstance(std::string name) const
{
    return std::make_unique<DrumSynth909Device>(std::move(name));
}

std::vector<std::string> DrumSynth909Device::kitNames()
{
    return { "Init", "Mono", "Equal", "Mono Equal" };
}

void DrumSynth909Device::loadKit(Kit kit)
{
    namespace NahdXml = Constants::NahdXml;
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
                gain *= std::pow(10.0, (EqualTrimDb.at(static_cast<size_t>(pad)) - MixTargetDb.at(static_cast<size_t>(pad))) / 20.0);
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

std::vector<DrumSynth909Device::Knob> DrumSynth909Device::voiceKnobs(int index)
{
    std::vector<Knob> knobs;
    for (auto && knob : padKnobs(index)) {
        knobs.push_back({ knob.key.toStdString(), knob.label });
    }
    return knobs;
}

void DrumSynth909Device::addEngineParameters(int index, const std::string & prefix)
{
    for (auto && knob : padKnobs(index)) {
        addParameter(Parameter { prefix + knob.key.toStdString(), KnobDefault, 0, 10000, static_cast<int>(KnobDefault * 10000), 100 });
    }
}

void DrumSynth909Device::syncEngineParameters(int index, const std::string & prefix)
{
    auto & engine = voiceEngine(index);
    for (auto && knob : padKnobs(index)) {
        if (const auto p = parameter(prefix + knob.key.toStdString()); p) {
            knob.set(engine, p->get().value());
        }
    }
}

std::pair<float, float> DrumSynth909Device::defaultVoiceMix(int index) const
{
    // Fader and pan as knob positions. The faders are set from each drum's measured peak to Drum
    // Synth V2's targets, each pad's peak against the kick: snare -2.5 dB, clap -3, toms -4, rim -6,
    // open hat -7, closed hat -8, ride and reverse crash -10; the crash at -9 rather than V2's -14,
    // which left it too quiet here. The pans are V2's.
    static constexpr std::array<std::pair<float, float>, NumVoices> mix { {
      { 0.7500f, 0.500f }, // Kick
      { 0.6662f, 0.300f }, // Snare
      { 0.2776f, 0.600f }, // ClosedHiHat
      { 0.6420f, 0.500f }, // Clap
      { 0.3206f, 0.350f }, // OpenHiHat
      { 0.5446f, 0.675f }, // LowTom
      { 0.5390f, 0.500f }, // MidTom
      { 0.5279f, 0.325f }, // HighTom
      { 0.2698f, 0.775f }, // Crash
      { 0.2458f, 0.700f }, // Ride
      { 0.2556f, 0.775f }, // ReverseCrash
      { 0.4131f, 0.300f }, // Rim
    } };
    return mix.at(static_cast<size_t>(index));
}

std::vector<std::string> DrumSynth909Device::modulationPriority() const
{
    return {
        NahdXml::xmlKeyCutoff().toStdString(),
        NahdXml::xmlKeyAmpDecay().toStdString(),
        NahdXml::xmlKeyTune().toStdString(),
        NahdXml::xmlKeyDecay().toStdString(),
        NahdXml::xmlKeyTone().toStdString(),
        NahdXml::xmlKeySnappy().toStdString(),
        NahdXml::xmlKeyHpfCutoff().toStdString(),
    };
}

} // namespace noteahead
