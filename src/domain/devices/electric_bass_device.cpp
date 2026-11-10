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

#include "electric_bass_device.hpp"

#include "device_parameter_display.hpp"
#include "electric_bass_presets.hpp"
#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

//! Inharmonicity of each open string, thickest first. A fret shortens the string and raises it by
//! the square of how much shorter the string is.
constexpr std::array<double, ElectricBassDevice::StringCount> OpenInharmonicity { 3.0e-4, 2.6e-4, 2.2e-4, 1.8e-4, 1.4e-4 };

constexpr int MaxFret = 24;

//! How a soft pluck's spectrum falls faster than a hard one's: fitted to the softer takes, which
//! fall about one power of the harmonic number faster 10 dB down
constexpr double HardTilt = 0.44;
constexpr double SoftTiltRange = 1.2;

//! Where the passive tone circuit puts the pickups' resonance and how sharp it is, from the tone
//! knob rolled off to open. The growlybass on 10 is the top, 4.3 kHz at a Q of 2.8; the darkblack's
//! 1.47 kHz at 0.6 falls on the same curve a quarter of the way up.
constexpr double ToneMinHz = 1000.0;
constexpr double ToneMaxHz = 4300.0;
constexpr double ToneMinQ = 0.5;
constexpr double ToneMaxQ = 2.8;

//! Brings a full-strength pluck of the open E to about the Bass Synth's level
constexpr double OutputGain = 0.07;

//! How fast the pitch follows the bend wheel, so a stepped controller still bends smoothly
constexpr double BendSmoothingSeconds = 0.005;
//! The strings are retuned this often while a bend moves
constexpr uint32_t BendInterval = 16;
//! Finger vibrato on the modulation wheel. A fretted string can only be pushed sharp, so the pitch
//! swings up from the note and back rather than either side of it.
constexpr double VibratoRateHz = 5.0;
//! How far up the wheel at the top pushes the string
constexpr double MaxVibratoCents = 40.0;
//! How fast the depth follows the wheel, so the vibrato swells in rather than steps
constexpr double VibratoSmoothingSeconds = 0.05;

struct CcParameter
{
    uint8_t controller {};
    std::string label;
    std::string key;
};

const std::vector<CcParameter> & ccParameters()
{
    using namespace MidiCcMapping;
    static const std::vector<CcParameter> parameters {
        { .controller = static_cast<uint8_t>(Controller::SoundController1), .label = "Position", .key = Xml::xmlKeyPosition().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController2), .label = "Tone", .key = Xml::xmlKeyTone().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController3), .label = "Release", .key = Xml::xmlKeyRelease().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController4), .label = "Finger", .key = Xml::xmlKeyHardness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController9), .label = "Brightness", .key = Xml::xmlKeyBrightness().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController6), .label = "Sustain", .key = Xml::xmlKeySustain().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController7), .label = "Neck", .key = Xml::xmlKeyNeckPickup().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController8), .label = "Bridge", .key = Xml::xmlKeyBridgePickup().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::EffectControl1MSB), .label = "Velocity Sensitivity", .key = Xml::xmlKeyVelocitySensitivity().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::SoundController5), .label = "LPF", .key = Xml::xmlKeyLpfCutoff().toStdString() },
        { .controller = static_cast<uint8_t>(Controller::GeneralPurpose6), .label = "HPF", .key = Xml::xmlKeyHpfCutoff().toStdString() },
    };
    return parameters;
}

float exponentialKnob(double value, double min, double max)
{
    return static_cast<float>(std::log(value / min) / std::log(max / min));
}

double noteHz(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

} // namespace

ElectricBassDevice::ElectricBassDevice(std::string name)
  : m_name { std::move(name) }
{
    const auto addKnob = [this](const QString & key, float defaultValue) {
        const auto defaultStep = static_cast<int>(std::lround(defaultValue * 10000.0f));
        addParameter(Parameter(key.toStdString(), defaultValue, 0, 10000, defaultStep, 100));
    };
    // The defaults are a Jazz Bass on both pickups with the tone open, plucked between them
    addKnob(Xml::xmlKeyPosition(), exponentialKnob(120.0, MinPluckMm, MaxPluckMm));
    addKnob(Xml::xmlKeyHardness(), static_cast<float>(std::log(10.8 / SoftFingerMm) / std::log(HardFingerMm / SoftFingerMm)));
    addKnob(Xml::xmlKeySustain(), exponentialKnob(23.0, MinSustainSeconds, MaxSustainSeconds));
    addKnob(Xml::xmlKeyBrightness(), static_cast<float>(std::log(0.0021 / DullLossPerHz) / std::log(BrightLossPerHz / DullLossPerHz)));
    addKnob(Xml::xmlKeyRelease(), exponentialKnob(0.12, MinReleaseSeconds, MaxReleaseSeconds));
    addKnob(Xml::xmlKeyNeckPickup(), 1.0f);
    addKnob(Xml::xmlKeyBridgePickup(), 1.0f);
    addKnob(Xml::xmlKeyTone(), 0.9f);
    addKnob(Xml::xmlKeyVelocitySensitivity(), 0.7f);
    addKnob(Xml::xmlKeyLpfCutoff(), 1.0f);
    addKnob(Xml::xmlKeyHpfCutoff(), 0.0f);
    addParameter(Parameter(Xml::xmlKeyPitchBendRange().toStdString(), 2.0f, 0, 24, 2, 1, Parameter::Type::Discrete));

    m_lpf.setMode(CascadedSvf::Mode::LowPass);
    m_hpf.setMode(CascadedSvf::Mode::HighPass);

    ElectricBassDevice::syncParameters();

    DeviceParameterDisplay::filters(*this);
    setParameterDisplay(Xml::xmlKeyPosition().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "mm", .min = MinPluckMm, .max = MaxPluckMm, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Position") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyHardness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Finger"));
    setParameterDisplay(Xml::xmlKeySustain().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = MinSustainSeconds, .max = MaxSustainSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Sustain") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBrightness(), QT_TRANSLATE_NOOP("ParameterDisplay", "Brightness"));
    setParameterDisplay(Xml::xmlKeyRelease().toStdString(), { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "s", .min = MinReleaseSeconds, .max = MaxReleaseSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Release") });
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyNeckPickup(), QT_TRANSLATE_NOOP("ParameterDisplay", "Neck"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyBridgePickup(), QT_TRANSLATE_NOOP("ParameterDisplay", "Bridge"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyTone(), QT_TRANSLATE_NOOP("ParameterDisplay", "Tone"));
    DeviceParameterDisplay::percent(*this, Xml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity Sensitivity"));
}

ElectricBassDevice::~ElectricBassDevice() = default;

std::string ElectricBassDevice::name() const
{
    return m_name;
}

std::string ElectricBassDevice::category() const
{
    return Xml::xmlValueSynths().toStdString();
}

std::string ElectricBassDevice::typeName() const
{
    return Constants::electricBassDeviceName().toStdString();
}

std::string ElectricBassDevice::typeIdString()
{
    return "8e3f1b6a-2c74-4d95-b0a1-6f2d9c4e7a13";
}

std::string ElectricBassDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> ElectricBassDevice::deviceMidiCcControllers() const
{
    std::vector<MidiCcController> controllers { { static_cast<uint8_t>(MidiCcMapping::Controller::ModulationWheelMSB), "Vibrato" } };
    for (const auto & parameter : ccParameters()) {
        controllers.push_back({ parameter.controller, parameter.label });
    }
    return controllers;
}

double ElectricBassDevice::pluckMm(float knob)
{
    return ParameterMapper::mapExponential(knob, MinPluckMm, MaxPluckMm);
}

double ElectricBassDevice::fingerMm(float knob)
{
    return ParameterMapper::mapExponential(knob, SoftFingerMm, HardFingerMm);
}

double ElectricBassDevice::sustainSeconds(float knob)
{
    return ParameterMapper::mapExponential(knob, MinSustainSeconds, MaxSustainSeconds);
}

double ElectricBassDevice::lossPerHz(float knob)
{
    return ParameterMapper::mapExponential(knob, DullLossPerHz, BrightLossPerHz);
}

double ElectricBassDevice::releaseSeconds(float knob)
{
    return ParameterMapper::mapExponential(knob, MinReleaseSeconds, MaxReleaseSeconds);
}

size_t ElectricBassDevice::chooseString(uint8_t note) const
{
    // A repeated note goes back to its own string
    for (size_t i = 0; i < StringCount; i++) {
        const auto & voice = m_voices.at(i);
        if (voice.note == note && voice.held.empty()) {
            return i;
        }
    }
    // Then the lowest fret that reaches the note, on a string the hand is not holding down
    std::optional<size_t> best;
    std::optional<size_t> bestHeld;
    for (size_t i = StringCount; i-- > 0;) {
        const int fret = note - OpenStrings.at(i);
        if (fret < 0 || fret > MaxFret) {
            continue;
        }
        if (m_voices.at(i).held.empty()) {
            if (!best) {
                best = i;
            }
        } else if (!bestHeld) {
            bestHeld = i;
        }
    }
    if (best) {
        return *best;
    }
    if (bestHeld) {
        return *bestHeld;
    }
    // Outside the neck: below the B it is the B tuned down, above the top fret the G stretched up
    return note < OpenStrings.front() ? 0 : StringCount - 1;
}

void ElectricBassDevice::startNote(size_t stringIndex, uint8_t note, float velocity, bool legato)
{
    auto & voice = m_voices.at(stringIndex);
    const double fret = static_cast<double>(note - OpenStrings.at(stringIndex));
    const double inharmonicity = OpenInharmonicity.at(stringIndex) * std::exp2(fret / 6.0);
    const double v = static_cast<double>(velocity);
    // A harder pluck is louder than in proportion: the recordings' mp is 10 dB under their f
    const double amplitude = v * v;
    const double tilt = HardTilt + SoftTiltRange * (1.0 - v);
    voice.string.setSettings(m_stringSettings);
    if (legato) {
        voice.string.legato(noteHz(note), fret, inharmonicity, amplitude, tilt);
    } else {
        voice.string.pluck(noteHz(note), fret, inharmonicity, amplitude, tilt);
    }
    voice.string.setPitchRatio(m_bend * m_vibratoRatio);
    voice.note = note;
    voice.velocity = velocity;
}

void ElectricBassDevice::processDeviceMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const float rawVelocity = static_cast<float>(velocity) / 127.0f;
    const float v = (1.0f - m_velocitySensitivity) + m_velocitySensitivity * rawVelocity;

    // A note within reach of one still held on its string is played on that string, without a new
    // pluck: a hammer-on above it, a pull-off below it
    for (size_t i = 0; i < StringCount; i++) {
        auto & voice = m_voices.at(i);
        if (voice.held.empty() || !voice.string.isActive()) {
            continue;
        }
        const int fret = note - OpenStrings.at(i);
        if (fret >= 0 && fret <= MaxFret && std::abs(note - voice.held.back()) <= LegatoReach && note != voice.held.back()) {
            voice.held.push_back(note);
            startNote(i, note, v, true);
            return;
        }
    }

    const size_t index = chooseString(note);
    auto & voice = m_voices.at(index);
    voice.held = { note };
    startNote(index, note, v, false);
}

void ElectricBassDevice::processDeviceMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (size_t i = 0; i < StringCount; i++) {
        auto & voice = m_voices.at(i);
        const auto it = std::ranges::find(voice.held, note);
        if (it == voice.held.end()) {
            continue;
        }
        const bool sounding = std::next(it) == voice.held.end();
        voice.held.erase(it);
        if (!sounding) {
            continue;
        }
        if (voice.held.empty()) {
            voice.string.mute();
        } else {
            // The finger lets go and the string sounds the fret still held under it
            startNote(i, voice.held.back(), voice.velocity, true);
        }
    }
}

void ElectricBassDevice::processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
            m_vibratoTarget = 0.0;
        } else {
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ModulationWheelMSB)) {
                // A performance gesture rather than a setting: nothing is saved, no knob moves
                m_vibratoTarget = static_cast<double>(val);
            } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else {
                const auto & parameters = ccParameters();
                if (const auto it = std::ranges::find(parameters, controller, &CcParameter::controller); it != parameters.end()) {
                    if (const auto p = parameter(it->key); p) {
                        p->get().setAutomationValue(val);
                        syncParameters();
                        changed = true;
                    }
                }
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void ElectricBassDevice::processMidiPitchBend(uint16_t value, uint8_t)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const double semitones = static_cast<double>(m_pitchBendRange) * (static_cast<double>(value) - 8192.0) / 8192.0;
    m_bendTarget = std::exp2(semitones / 12.0);
}

void ElectricBassDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.held.clear();
        voice.string.mute();
    }
}

void ElectricBassDevice::updateTone(double sampleRate)
{
    const double tone = static_cast<double>(value(Xml::xmlKeyTone()));
    const double hz = ToneMinHz * std::pow(ToneMaxHz / ToneMinHz, tone);
    const double q = ToneMinQ + (ToneMaxQ - ToneMinQ) * tone * tone;
    if (hz != m_toneHz || q != m_toneQ || sampleRate != m_toneSampleRate) {
        m_toneHz = hz;
        m_toneQ = q;
        m_toneSampleRate = sampleRate;
        m_tone.calculateHighCut(std::min(hz, 0.45 * sampleRate), sampleRate, q);
    }
}

void ElectricBassDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const auto sampleRate = static_cast<double>(context.sampleRate);
    for (auto && voice : m_voices) {
        voice.string.setSampleRate(sampleRate);
    }
    m_dcBlockerL.setSampleRate(context.sampleRate);
    m_dcBlockerR.setSampleRate(context.sampleRate);
    updateTone(sampleRate);
    m_lpf.setSampleRate(context.sampleRate);
    m_hpf.setSampleRate(context.sampleRate);
    m_lpf.setCutoff(static_cast<double>(m_lpfCutoff));
    m_hpf.setCutoff(static_cast<double>(m_hpfCutoff));
    m_panner.setPan(static_cast<double>(panInternal()));

    const double gain = linearGainInternal() * OutputGain;
    const double bendCoefficient = 1.0 - std::exp(-static_cast<double>(BendInterval) / (BendSmoothingSeconds * sampleRate));
    const double vibratoCoefficient = 1.0 - std::exp(-static_cast<double>(BendInterval) / (VibratoSmoothingSeconds * sampleRate));
    const double vibratoPhaseStep = 2.0 * std::numbers::pi * VibratoRateHz * static_cast<double>(BendInterval) / sampleRate;

    for (uint32_t i = 0; i < context.frameCount; i++) {
        if (i % BendInterval == 0 && (m_bend != m_bendTarget || m_vibratoDepth > 0.0 || m_vibratoTarget > 0.0)) {
            if (m_bend != m_bendTarget) {
                m_bend += (m_bendTarget - m_bend) * bendCoefficient;
                if (std::abs(m_bend - m_bendTarget) < 1.0e-6) {
                    m_bend = m_bendTarget;
                }
            }
            updateVibrato(vibratoCoefficient, vibratoPhaseStep);
            for (auto && voice : m_voices) {
                voice.string.setPitchRatio(m_bend * m_vibratoRatio);
            }
        }

        double mono = 0.0;
        for (auto && voice : m_voices) {
            mono += voice.string.nextSample();
        }
        mono = m_hpf.process(m_lpf.process(m_tone.process(mono))) * gain;

        double left = 0.0;
        double right = 0.0;
        m_panner.processMono(mono, left, right);
        context.buffer[i * 2] += m_dcBlockerL.process(left);
        context.buffer[i * 2 + 1] += m_dcBlockerR.process(right);
    }
}

bool ElectricBassDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return std::ranges::any_of(m_voices, [](const Voice & voice) { return voice.string.isActive(); });
}

std::optional<size_t> ElectricBassDevice::stringOf(uint8_t note) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (size_t i = 0; i < StringCount; i++) {
        if (m_voices.at(i).note == note && m_voices.at(i).string.isActive()) {
            return i;
        }
    }
    return {};
}

void ElectricBassDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
    syncParameters();
}

void ElectricBassDevice::resetDeviceAudio()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        voice.string.reset();
        voice.held.clear();
        voice.note.reset();
    }
    m_tone.reset();
    m_lpf.reset();
    m_hpf.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_bend = m_bendTarget = 1.0;
    m_vibratoTarget = m_vibratoDepth = m_vibratoPhase = 0.0;
    m_vibratoRatio = 1.0;
}

void ElectricBassDevice::updateVibrato(double coefficient, double phaseStep)
{
    m_vibratoDepth += (m_vibratoTarget - m_vibratoDepth) * coefficient;
    if (m_vibratoTarget == 0.0 && m_vibratoDepth < 1.0e-4) {
        // At rest: the next vibrato starts from the note itself rather than partway up a swing
        m_vibratoDepth = m_vibratoPhase = 0.0;
        m_vibratoRatio = 1.0;
        return;
    }
    m_vibratoPhase = std::fmod(m_vibratoPhase + phaseStep, 2.0 * std::numbers::pi);
    const double cents = m_vibratoDepth * MaxVibratoCents * 0.5 * (1.0 - std::cos(m_vibratoPhase));
    m_vibratoRatio = std::exp2(cents / 1200.0);
}

void ElectricBassDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Xml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeStartElement(Xml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Xml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement();
}

void ElectricBassDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Xml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Xml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Xml::xmlKeyMidiMappings()) {
                    deserializeMidiMappingsFromXml(reader);
                } else if (reader.name() == Xml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == Xml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

void ElectricBassDevice::loadPreset(int index)
{
    applyPreset(index, true);
}

void ElectricBassDevice::applyPreset(int index, bool authored)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        const auto & presets = ElectricBassPresets::presets();
        if (index < 0 || index >= static_cast<int>(presets.size())) {
            return;
        }

        if (authored) {
            reset();
        } else {
            for (auto && [name, p] : parameters()) {
                p.setAutomationValue(p.defaultValue());
            }
        }

        for (auto && [name, value] : presets[index].parameters) {
            if (const auto p = parameter(name); p) {
                if (authored) {
                    p->get().setValue(value);
                } else {
                    p->get().setAutomationValue(value);
                }
            }
        }

        syncParameters();
    }

    if (authored) {
        emit dataChanged();
    } else {
        emit parametersChanged();
    }
}

float ElectricBassDevice::value(const QString & key) const
{
    if (const auto p = parameter(key.toStdString()); p) {
        return p->get().value();
    }
    return 0.0f;
}

float ElectricBassDevice::knob(const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return shownParameterValue(key, value(key));
}

void ElectricBassDevice::setKnob(const QString & key, float value)
{
    setContinuousParameterValue(key.toStdString(), value);
}

int ElectricBassDevice::pitchBendRange() const
{
    return m_pitchBendRange;
}

void ElectricBassDevice::setPitchBendRange(int range)
{
    setDiscreteParameterValue(Xml::xmlKeyPitchBendRange().toStdString(), range);
}

void ElectricBassDevice::syncParameters()
{
    Device::syncParameters();
    m_stringSettings = {
        .pluckMm = pluckMm(value(Xml::xmlKeyPosition())),
        .fingerMm = fingerMm(value(Xml::xmlKeyHardness())),
        .fundamentalT60 = sustainSeconds(value(Xml::xmlKeySustain())),
        .lossPerHz = lossPerHz(value(Xml::xmlKeyBrightness())),
        .muteT60 = releaseSeconds(value(Xml::xmlKeyRelease())),
        .neck = static_cast<double>(value(Xml::xmlKeyNeckPickup())),
        .bridge = static_cast<double>(value(Xml::xmlKeyBridgePickup())),
    };
    // The decay and the pickups change under a ringing string; where the finger plucks only
    // matters to the next pluck
    for (auto && voice : m_voices) {
        voice.string.setSettings(m_stringSettings);
    }
    m_velocitySensitivity = value(Xml::xmlKeyVelocitySensitivity());
    m_lpfCutoff = value(Xml::xmlKeyLpfCutoff());
    m_hpfCutoff = value(Xml::xmlKeyHpfCutoff());
    if (const auto p = parameter(Xml::xmlKeyPitchBendRange().toStdString()); p) {
        m_pitchBendRange = static_cast<int>(p->get().xmlValue());
    }
}

std::vector<std::string> ElectricBassDevice::modulationPriority() const
{
    return {
        Xml::xmlKeyTone().toStdString(),
        Xml::xmlKeyPosition().toStdString(),
        Xml::xmlKeyBridgePickup().toStdString(),
        Xml::xmlKeyNeckPickup().toStdString(),
        Xml::xmlKeyRelease().toStdString(),
        Xml::xmlKeyHardness().toStdString(),
        Xml::xmlKeyBrightness().toStdString(),
        Xml::xmlKeySustain().toStdString(),
        Xml::xmlKeyLpfCutoff().toStdString(),
        Xml::xmlKeyHpfCutoff().toStdString()
    };
}

} // namespace noteahead
