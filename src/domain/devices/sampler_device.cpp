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

#include "sampler_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/audio/backend/sndfile_reader.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iomanip>
#include <stdexcept>

#include <QDir>
#include <QFileInfo>
#include <QVariant>

namespace noteahead {

static const auto TAG = "SamplerDevice";

SamplerDevice::Sample::Sample()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Device::faderParameter());
    volume = Constants::faderUnityPosition();
    addParameter(Parameter { Constants::NahdXml::xmlKeyCutoff().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStartOffset().toStdString(), 0.0f, 0, 60000, 0, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyEndOffset().toStdString(), 0.0f, 0, 60000, 0, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyTune().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDetune().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDecay().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySustain().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverse().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLoop().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLoopStart().toStdString(), 0.0f, 0, 60000, 0, 1 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyChokeGroup().toStdString(), 0.0f, 0, maxChokeGroup, 0, 1, Parameter::Type::Discrete });
}

namespace {

//! Seconds each envelope segment takes at the given knob position.
//!
//! Attack starts at nothing, so a pad that never touches it triggers at full level on its first frame
//! the way it always has. The other two bottom out at the de-click fade the pads used to apply on
//! note-off, which is the shortest release worth having.
constexpr double MinEnvelopeSeconds { 0.005 };
constexpr double MaxEnvelopeSeconds { 10.0 };

//! How long a choked voice takes to fall silent. Long enough not to click, short enough that the pad
//! taking over is not audibly sharing the bar with the one it cut.
constexpr double ChokeFadeSeconds { 0.005 };

//! Range of the coarse tuning knob, in whole semitones either way.
constexpr int TuneSemitoneRange { 24 };
//! Range of the fine tuning knob, in cents either way.
constexpr double DetuneCentRange { 100.0 };

} // namespace

double SamplerDevice::attackSeconds(float attack)
{
    return attack <= 0.0f ? 0.0 : ParameterMapper::mapExponential(static_cast<double>(attack), MinEnvelopeSeconds, MaxEnvelopeSeconds);
}

double SamplerDevice::decaySeconds(float decay)
{
    return ParameterMapper::mapExponential(static_cast<double>(decay), MinEnvelopeSeconds, MaxEnvelopeSeconds);
}

double SamplerDevice::releaseSeconds(float release)
{
    return ParameterMapper::mapExponential(static_cast<double>(release), MinEnvelopeSeconds, MaxEnvelopeSeconds);
}

int SamplerDevice::tuneSemitones(float tune)
{
    return static_cast<int>(std::lround((static_cast<double>(tune) - 0.5) * 2.0 * TuneSemitoneRange));
}

double SamplerDevice::detuneCents(float detune)
{
    return (static_cast<double>(detune) - 0.5) * 2.0 * DetuneCentRange;
}

double SamplerDevice::tuneRatio(const Sample & sample)
{
    const double semitones = static_cast<double>(tuneSemitones(sample.tune)) + detuneCents(sample.detune) / 100.0;
    return std::pow(2.0, semitones / 12.0);
}

std::optional<SamplerDevice::PlayRange> SamplerDevice::playRange(const Sample & sample)
{
    if (!sample.data || sample.channels <= 0 || sample.sampleRate <= 0) {
        return std::nullopt;
    }

    const size_t totalFrames = sample.data->size() / static_cast<size_t>(sample.channels);
    if (totalFrames < 2) {
        return std::nullopt;
    }

    const double lastFrame = static_cast<double>(totalFrames - 1);
    const double rate = static_cast<double>(sample.sampleRate);

    // Both offsets are trims counted in from their own end of the waveform, so either one at zero
    // takes nothing off. A reversed pad is heard back to front, which means the start offset cuts into
    // the tail of the file and the end offset into its head: the two simply swap ends.
    const double startTrim = std::clamp(sample.startOffset * rate, 0.0, lastFrame);
    const double endTrim = std::clamp(sample.endOffset * rate, 0.0, lastFrame);
    double first = sample.reverse ? endTrim : startTrim;
    double last = lastFrame - (sample.reverse ? startTrim : endTrim);

    // A loop wraps on whole frames. The offsets are kept as normalised floats, which lands them up to
    // a fraction of a frame either side of where they were dialled in -- enough for the seam check to
    // miss and let the loop read the frame past its own end on every wrap.
    if (sample.loop) {
        first = std::floor(first);
        last = std::floor(last);
    }

    // A range that ends where it starts, or before it, has nothing to play.
    if (last <= first) {
        return std::nullopt;
    }

    // The loop point is counted in from the end the pad starts playing from, which is the far end of
    // the range when it plays reversed. Keeping at least a frame of loop leaves a pad whose loop point
    // sits on the very end of its range something to repeat instead of falling silent on the first wrap.
    double loopFirst = first;
    double loopLast = last;
    if (sample.loop && sample.loopStart > 0.0) {
        const double loopTrim = std::floor(std::clamp(sample.loopStart * rate, 0.0, last - first - 1.0));
        if (sample.reverse) {
            loopLast = last - loopTrim;
        } else {
            loopFirst = first + loopTrim;
        }
    }

    return PlayRange { first, last, loopFirst, loopLast };
}

void SamplerDevice::updateVoiceEnvelope(Voice & voice)
{
    if (!voice.sample) {
        return;
    }
    voice.ampEg.setAttackTime(attackSeconds(voice.sample->attack));
    voice.ampEg.setDecayTime(decaySeconds(voice.sample->decay));
    voice.ampEg.setSustainLevel(static_cast<double>(voice.sample->sustain));
    voice.ampEg.setReleaseTime(releaseSeconds(voice.sample->release));
}

void SamplerDevice::wrapPosition(Voice & voice, const PlayRange & range)
{
    // The loop is half-open. The frame at the far end is where the loop comes back round to, not
    // material the loop plays, so the length is the plain distance between the two ends and a position
    // that reaches the end has already wrapped.
    //
    // The wrap is triggered by the range and lands inside the loop, which is what lets a pad whose loop
    // point sits inside its range play the head once and then repeat only the tail.
    //
    // fmod rather than a subtraction: a pad tuned two octaves up steps four frames at a time, and a
    // short loop can be jumped clean over.
    const double length = range.loopLast - range.loopFirst;
    if (length <= 0.0) {
        return;
    }
    // Which end the voice can run off is decided by the direction it plays in, not by where it
    // happens to be. A reversed voice starts on the far end of its range, and testing that end would
    // read it as having already run past it and wrap it away before it has played a single frame.
    if (voice.sample->reverse) {
        if (voice.position < range.first) {
            voice.position = range.loopLast - std::fmod(range.loopLast - voice.position, length);
        }
    } else if (voice.position >= range.last) {
        voice.position = range.loopFirst + std::fmod(voice.position - range.loopFirst, length);
    }
}

void SamplerDevice::chokeVoicesOf(const Sample & trigger)
{
    if (trigger.chokeGroup <= 0) {
        return;
    }
    for (auto && voice : m_voices) {
        if (voice.active && voice.sample && voice.sample != &trigger && voice.sample->chokeGroup == trigger.chokeGroup) {
            voice.choking = true;
        }
    }
}

SamplerDevice::SamplerDevice(std::string name, AudioFileReaderU audioFileReader)
  : m_name { std::move(name) }
  , m_audioFileReader { audioFileReader ? std::move(audioFileReader) : std::make_unique<SndFileReader>() }
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyChannelMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyChromaticMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { Constants::NahdXml::xmlKeyEmbedWaveData().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    m_voices.resize(m_maxVoices);
    for (auto && sample : m_samples) {
        sample = nullptr;
    }

    SamplerDevice::syncParameters();
}

SamplerDevice::~SamplerDevice() = default;

SamplerDevice::Voice::Voice()
{
    lpf = std::make_shared<LowPassFilter>();
    hpf = std::make_shared<HighPassFilter>();
    volumeEffect = std::make_shared<Volume>();
    panningEffect = std::make_shared<Panning>();
    effects = { lpf, hpf, volumeEffect, panningEffect };
}

void SamplerDevice::updateVoiceEffects(Voice & voice)
{
    const float sPan = (voice.sample->pan * 2.0f) - 1.0f;
    const float mPan = (voice.pan * 2.0f) - 1.0f;
    const float combinedPan = (std::clamp(sPan + mPan, -1.0f, 1.0f) + 1.0f) / 2.0f;
    voice.panningEffect->setPan(combinedPan);

    const float combinedVolume = static_cast<float>(ParameterMapper::mapFader(voice.sample->volume)) * voice.velocity;
    voice.volumeEffect->setVolume(combinedVolume);

    voice.lpf->setCutoff(std::clamp(voice.sample->cutoff + (voice.cutoff - 1.0f), 0.0f, 1.0f));
    voice.hpf->setCutoff(std::clamp(voice.sample->hpfCutoff + voice.hpfCutoff, 0.0f, 1.0f));
}

std::string SamplerDevice::name() const
{
    return m_name;
}

std::string SamplerDevice::category() const
{
    return Constants::NahdXml::xmlValueSamplers().toStdString();
}

std::string SamplerDevice::typeName() const
{
    return Constants::samplerDeviceName().toStdString();
}

std::string SamplerDevice::typeIdString()
{
    return "9dda4ff6-471b-11f1-9324-c701bfaf8258";
}

std::string SamplerDevice::typeId() const
{
    return typeIdString();
}

std::vector<MidiCcController> SamplerDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    std::vector<MidiCcController> list {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" },
        { static_cast<uint8_t>(Controller::SoundController5), "LPF" },
        { static_cast<uint8_t>(Controller::GeneralPurpose6), "HPF" }
    };

    // Pads are named by index rather than by note, because the index is what stays put when chromatic
    // mode remaps the notes. The note rides along separately for the presentation layer to name.
    for (int pad { 0 }; pad < padCount; pad++) {
        const int note = noteForPad(pad);
        // Chromatic mode runs the topmost pads past the end of the keyboard: those address nothing
        const std::optional<uint8_t> padNote = note < static_cast<int>(maxSamples)
          ? std::optional<uint8_t> { static_cast<uint8_t>(note) }
          : std::nullopt;
        const auto padController = [pad, padNote](int number, const std::string & parameterName, int maxValue = 127) {
            return MidiCcController { static_cast<uint8_t>(number), std::format("Pad {} {}", pad + 1, parameterName), 0, maxValue, padNote };
        };
        list.push_back(padController(padPanCcStart + pad, "Pan"));
        // The pad fader reaches past unity just like the device-wide one, so it needs the same range.
        list.push_back(padController(padVolumeCcStart + pad, "Volume", Constants::faderMaxMidiCcValue()));
        list.push_back(padController(padCutoffCcStart + pad, "LPF"));
        list.push_back(padController(padHpfCutoffCcStart + pad, "HPF"));
    }

    return list;
}

int SamplerDevice::noteForPad(int padIndex) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    if (m_chromaticMode) {
        return padIndex * 12; // Each pad is an octave; its root is the C of that octave.
    }
    return padStartNote + padIndex;
}

std::optional<SamplerDevice::PadCcTarget> SamplerDevice::padCcTarget(uint8_t controller, uint8_t value) const
{
    const auto inBlock = [controller](uint8_t start) {
        return controller >= start && controller < start + padCount;
    };

    const float mapped = static_cast<float>(value) / 127.0f;

    if (inBlock(padPanCcStart)) {
        return PadCcTarget { controller - padPanCcStart, Constants::NahdXml::xmlKeyPan().toStdString(), mapped };
    }
    if (inBlock(padVolumeCcStart)) {
        return PadCcTarget { controller - padVolumeCcStart, Constants::NahdXml::xmlKeyFader().toStdString(), faderPositionFromMidiCc(value) };
    }
    if (inBlock(padCutoffCcStart)) {
        return PadCcTarget { controller - padCutoffCcStart, Constants::NahdXml::xmlKeyCutoff().toStdString(), mapped };
    }
    if (inBlock(padHpfCutoffCcStart)) {
        return PadCcTarget { controller - padHpfCutoffCcStart, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), mapped };
    }

    return std::nullopt;
}

bool SamplerDevice::updatePadParameter(int note, const std::string & parameterName, float value)
{
    if (note < 0 || note >= static_cast<int>(maxSamples) || !m_samples.at(static_cast<size_t>(note))) {
        return false;
    }

    auto & sample = *m_samples.at(static_cast<size_t>(note));

    if (parameterName == Constants::NahdXml::xmlKeyPan().toStdString()) {
        sample.pan = value;
    } else if (parameterName == Constants::NahdXml::xmlKeyFader().toStdString()) {
        sample.volume = value;
    } else if (parameterName == Constants::NahdXml::xmlKeyCutoff().toStdString()) {
        sample.cutoff = value;
    } else if (parameterName == Constants::NahdXml::xmlKeyHpfCutoff().toStdString()) {
        sample.hpfCutoff = value;
    } else {
        return false;
    }

    // MIDI CC only: the pad dialog writes its parameters straight, so this never authors a value.
    if (auto p = sample.parameter(parameterName); p) {
        p->get().setAutomationValue(value);
    }

    // Only the pad's own value changed. updateVoiceEffects() combines it with the device-wide pan and
    // cutoff held in the voice, which have to be left as they are or the pad value lands twice.
    for (auto && voice : m_voices) {
        if (voice.active && voice.sample == &sample) {
            updateVoiceEffects(voice);
        }
    }

    return true;
}

void SamplerDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };

    if (note >= maxSamples) {
        return;
    }

    // In chromatic mode the note is pitched from the covering octave-root sample; otherwise each note plays
    // its own dedicated sample (drum style).
    Sample * sample = nullptr;
    double pitchRatio = 1.0;
    if (m_chromaticMode) {
        uint8_t rootNote = 0;
        sample = const_cast<Sample *>(coveringSample(note, rootNote));
        if (sample) {
            pitchRatio = std::pow(2.0, (static_cast<double>(note) - static_cast<double>(rootNote)) / 12.0);
        }
    } else if (m_samples.at(note)) {
        sample = m_samples.at(note).get();
    }

    if (!sample) {
        return;
    }

    // Stop any existing voices playing the same note (monophonic per note for now)
    // For de-clicking, we immediately stop it and let the new one start
    // A better way would be to let it finish its fade, but we need to find a free voice instead.
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.active = false;
        }
    }

    // A pad trimmed down to nothing, or too short to interpolate across, has nothing to trigger.
    const auto range = playRange(*sample);
    if (!range) {
        return;
    }

    chokeVoicesOf(*sample);

    // Find an inactive voice
    for (auto && voice : m_voices) {
        if (!voice.active) {
            voice.note = note;
            voice.sample = sample;
            voice.pitchRatio = pitchRatio;
            // Reverse plays the same range, from the far end of it.
            voice.position = sample->reverse ? range->last : range->first;
            voice.velocity = static_cast<float>(velocity) / 127.0f;
            voice.pan = panInternal();
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            for (auto && effect : voice.effects) {
                effect->reset();
            }

            updateVoiceEffects(voice);

            voice.ampEg.setSampleRate(static_cast<double>(sampleRate()));
            voice.ampEg.reset();
            updateVoiceEnvelope(voice);
            voice.ampEg.trigger();

            voice.choking = false;
            voice.chokeGain = 1.0f;
            voice.active = true;
            return;
        }
    }
}

void SamplerDevice::processMidiNoteOff(uint8_t note)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && voice : m_voices) {
        if (voice.active && voice.note == note) {
            voice.ampEg.release();
        }
    }
}

void SamplerDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t channel)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            changed |= clearAutomationInternal();
        } else if (const auto target = padCcTarget(controller, value); target) {
            // The per-pad CC blocks are disjoint from every controller the modes below read, so they
            // address a pad the same way whether or not channel mode is on.
            changed |= updatePadParameter(noteForPad(target->padIndex), target->parameterName, target->value);
        } else if (m_channelMode) {
            // channel is 0-indexed (0-15). Unlike the per-pad CCs above this keeps the drum layout even
            // in chromatic mode, which is how the mode has always addressed its pads.
            const int note = padStartNote + channel;
            const float val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::PanMSB)) { // Panning
                changed |= updatePadParameter(note, Constants::NahdXml::xmlKeyPan().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) { // Volume
                changed |= updatePadParameter(note, Constants::NahdXml::xmlKeyFader().toStdString(), faderPositionFromMidiCc(value));
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff (LPF)
                changed |= updatePadParameter(note, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // General Purpose 6 (HPF)
                changed |= updatePadParameter(note, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            }
        } else {
            if (controller == static_cast<uint8_t>(Controller::PanMSB)) { // Panning
                changed |= updatePanParameter(static_cast<float>(value) / 127.0f, false);
                // Update all active voices' pan
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.pan = panInternal();
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) { // Volume
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
                // Update all active voices' volume
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // Cutoff (LPF)
                m_globalCutoff = static_cast<float>(value) / 127.0f;
                // Update all active voices' cutoff
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.cutoff = m_globalCutoff;
                        updateVoiceEffects(voice);
                    }
                }
            } else if (controller == static_cast<uint8_t>(Controller::GeneralPurpose6)) { // General Purpose 6 (HPF)
                m_globalHpfCutoff = static_cast<float>(value) / 127.0f;
                // Update all active voices' hpfCutoff
                for (auto && voice : m_voices) {
                    if (voice.active) {
                        voice.hpfCutoff = m_globalHpfCutoff;
                        updateVoiceEffects(voice);
                    }
                }
            }
        }
    }

    if (changed) {
        emit parametersChanged();
    }
}

void SamplerDevice::processMidiAllNotesOff()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        for (auto && voice : m_voices) {
            voice.active = false;
        }

        clearAutomationInternal();
    }

    // Transport traffic, not an edit: this runs on every stop, so routing it through dataChanged()
    // marked the project modified just for playing it.
    emit parametersChanged();
}

void SamplerDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const uint32_t bufferSize = context.frameCount * 2;

    // Reuse the member scratch buffer instead of allocating on every audio callback.
    if (m_mixBuffer.size() < bufferSize) {
        m_mixBuffer.resize(bufferSize);
    }
    std::fill(m_mixBuffer.begin(), m_mixBuffer.begin() + bufferSize, 0.0);
    std::vector<double> & buffer = m_mixBuffer;

    // Per-pad sub-mix buffers for samples carrying a non-empty insert rack. Voices are grouped by their
    // Sample so a single stateful rack instance processes the summed signal of that pad (correct also in
    // chromatic mode, where several notes share one covering sample). m_padBuffers is a reusable pool;
    // only the first padCount entries are used this callback, so no allocation happens after warmup.
    size_t padCount = 0;
    const auto padBufferFor = [&](Sample * sample) -> std::vector<double> & {
        for (size_t k = 0; k < padCount; k++) {
            if (m_padBuffers[k].first == sample) {
                return m_padBuffers[k].second;
            }
        }
        if (padCount >= m_padBuffers.size()) {
            m_padBuffers.emplace_back(nullptr, std::vector<double>(bufferSize, 0.0));
        }
        auto & slot = m_padBuffers[padCount++];
        slot.first = sample;
        if (slot.second.size() < bufferSize) {
            slot.second.resize(bufferSize);
        }
        std::fill(slot.second.begin(), slot.second.begin() + bufferSize, 0.0);
        return slot.second;
    };

    const double gain = static_cast<double>(linearGainInternal());

    for (auto && voice : m_voices) {
        if (!voice.active || !voice.sample || !voice.sample->data) {
            continue;
        }

        // Read afresh every callback rather than latched at note-on, so trimming or tuning a pad in the
        // dialog is audible on the note that is already sounding.
        const auto range = playRange(*voice.sample);
        if (!range) {
            voice.active = false;
            continue;
        }

        for (auto && effect : voice.effects) {
            effect->setSampleRate(context.sampleRate);
        }

        voice.ampEg.setSampleRate(context.sampleRate);
        updateVoiceEnvelope(voice);

        // Rack pads accumulate dry (unity gain) into their own buffer; device gain is applied when the
        // processed pad buffer is folded into the main output. Rack-less pads take the direct fast path.
        const bool hasRack = voice.sample->effectRack && voice.sample->effectRack->hasEffects();
        std::vector<double> & target = hasRack ? padBufferFor(voice.sample) : buffer;
        const double voiceGain = hasRack ? 1.0 : gain;

        const auto & sampleData { *voice.sample->data };
        const int channels = voice.sample->channels;
        const size_t lastFrame = sampleData.size() / static_cast<size_t>(channels) - 1;
        const double rateScale = static_cast<double>(voice.sample->sampleRate) / static_cast<double>(context.sampleRate);
        const double pitchScale = rateScale * voice.pitchRatio * tuneRatio(*voice.sample)
          * (voice.sample->reverse ? -1.0 : 1.0);
        const bool loops = voice.sample->loop;
        const float chokeStep = static_cast<float>(1.0 / (ChokeFadeSeconds * context.sampleRate));

        for (uint32_t i = 0; i < context.frameCount; i++) {
            // Wrapped before the frame is read rather than after it is stepped, so that a loop pulled
            // shorter while it is sounding takes the voice with it instead of running it off the end.
            if (loops) {
                wrapPosition(voice, *range);
            }

            const double currentPos = voice.position;

            if (currentPos < range->first || currentPos > range->last) {
                voice.active = false;
                break;
            }

            const size_t index = static_cast<size_t>(currentPos);
            const float fract = static_cast<float>(currentPos - index);
            // A loop interpolates its last frame towards the frame it wraps round to, not towards the
            // one that follows it in the file: reading across the seam is what makes a loop buzz at its
            // own rate. Without a loop the final frame has no successor and interpolates towards itself.
            const size_t next = (loops && static_cast<double>(index + 1) >= range->last)
              ? static_cast<size_t>(range->loopFirst)
              : std::min(index + 1, lastFrame);

            double left = 0.0;
            double right = 0.0;

            if (channels == 1) {
                const double s0 = static_cast<double>(sampleData.at(index));
                const double s1 = static_cast<double>(sampleData.at(next));
                left = right = s0 + (s1 - s0) * fract;
            } else if (channels == 2) {
                const double l0 = static_cast<double>(sampleData.at(index * 2));
                const double l1 = static_cast<double>(sampleData.at(next * 2));
                const double r0 = static_cast<double>(sampleData.at(index * 2 + 1));
                const double r1 = static_cast<double>(sampleData.at(next * 2 + 1));
                left = l0 + (l1 - l0) * fract;
                right = r0 + (r1 - r0) * fract;
            }

            for (auto && effect : voice.effects) {
                effect->process(left, right);
            }

            const double envelope = voice.ampEg.nextSample();
            left *= envelope;
            right *= envelope;

            if (voice.choking) {
                left *= static_cast<double>(voice.chokeGain);
                right *= static_cast<double>(voice.chokeGain);
                voice.chokeGain -= chokeStep;
            }

            target[i * 2] += left * voiceGain;
            target[i * 2 + 1] += right * voiceGain;

            // A percussive envelope parks at a zero sustain rather than going idle, so a voice held by a
            // pattern that never releases it would otherwise render silence at full cost forever. A
            // looping voice never runs off its range, so this and the choke are all that can end it.
            if (voice.ampEg.isSilent() || (voice.choking && voice.chokeGain <= 0.0f)) {
                voice.active = false;
                voice.choking = false;
                break;
            }

            voice.position += pitchScale;
        }
    }

    // Apply each pad's insert rack to its sub-mix and fold it into the main buffer with device gain.
    for (size_t k = 0; k < padCount; k++) {
        auto & [sample, padBuffer] = m_padBuffers[k];
        auto & rack = *sample->effectRack;
        rack.setBpm(static_cast<float>(context.bpm));
        AudioContext padContext { std::span<double>(padBuffer.data(), bufferSize), context.frameCount, context.sampleRate, context.bpm, {}, context.oversampleFactor, context.offline };
        rack.processInPlace(padContext);
        for (uint32_t i = 0; i < bufferSize; i++) {
            buffer[i] += padBuffer[i] * gain;
        }
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        context.buffer[i * 2] += buffer[i * 2];
        context.buffer[i * 2 + 1] += buffer[i * 2 + 1];
    }
}

bool SamplerDevice::hasActiveAudio() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (const auto & voice : m_voices) {
        if (voice.active && voice.sample && voice.sample->data) {
            return true;
        }
    }
    return false;
}

void SamplerDevice::reset()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        Device::reset();
        stopVoicesUsing(nullptr);
        for (auto && sample : m_samples) {
            sample = nullptr;
        }
        m_globalCutoff = 1.0f;
        m_globalHpfCutoff = 0.0f;
        m_authoredGlobalCutoff = 1.0f;
        m_authoredGlobalHpfCutoff = 0.0f;
        resetAudio();
        syncParameters();
    }

    emit dataChanged();
}

void SamplerDevice::stopVoicesUsing(const Sample * sample)
{
    for (auto && voice : m_voices) {
        if (voice.sample && (!sample || voice.sample == sample)) {
            voice.active = false;
            voice.choking = false;
            voice.sample = nullptr;
        }
    }
}

void SamplerDevice::resetAudio()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        for (auto && voice : m_voices) {
            voice.active = false;
        }
    }
}

void SamplerDevice::loadSample(uint8_t note, const std::string & filePath)
{
    if (note >= maxSamples) {
        return;
    }

    const auto absolutePath = [this, &filePath]() {
        const auto path = QString::fromStdString(filePath);
        const auto resolvedPath = m_pathResolver ? m_pathResolver(path) : path;
        return (QFileInfo { resolvedPath }.isRelative() && !m_projectPath.empty())
          ? QFileInfo { QDir { QString::fromStdString(m_projectPath) }, resolvedPath }.absoluteFilePath()
          : resolvedPath;
    }();

    // We always reload the sample to support cases where the file on disk has changed (e.g. after recording)
    AudioFileReader::Info info {};
    juzzlin::L(TAG).info() << "Loading sample " << std::quoted(absolutePath.toStdString());
    if (!m_audioFileReader->open(absolutePath.toStdString(), AudioFileReader::Mode::Read, info)) {
        std::stringstream ss;
        ss << "Loading sample failed: " << std::quoted(absolutePath.toStdString());
        throw std::runtime_error { ss.str() };
    }

    const auto data = std::make_shared<std::vector<float>>();
    data->resize(static_cast<size_t>(info.frames * info.channels));
    m_audioFileReader->readFloat(std::span<float> { *data });
    m_audioFileReader->close();

    auto sample = std::make_unique<Sample>();
    if (QString::fromStdString(filePath).startsWith(Constants::NahdXml::embeddedDataPathPrefix())) {
        sample->filePath = filePath;
    } else {
        sample->filePath = absolutePath.toStdString();
    }
    sample->channels = info.channels;
    sample->sampleRate = info.samplerate;
    sample->data = std::move(data);

    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        // A pad that already holds something keeps everything but the audio: its insert rack, and
        // the settings it was dialled in with. Putting a different file on a pad is a substitution,
        // not a fresh start -- re-recording onto it, and Change File, both mean "this pad, that
        // sound". Loading onto an empty pad has nothing to carry and starts from the defaults.
        if (auto & existing = m_samples.at(note); existing) {
            static_cast<ParameterContainer &>(*sample) = static_cast<const ParameterContainer &>(*existing);
            syncSampleFields(*sample);
            sample->effectRack = std::move(existing->effectRack);
        }

        // The two offsets are the settings that are about the file rather than the pad, so a shorter
        // replacement has to pull them back inside the sample instead of pointing past its end.
        if (const auto duration = sampleDurationOf(*sample); duration > 0.0) {
            const auto clampOffset = [&sample, duration](const QString & key, double offset) {
                if (offset > duration) {
                    if (auto p = sample->parameter(key.toStdString()); p) {
                        p->get().setValue(static_cast<float>(duration / 60.0));
                    }
                }
            };
            clampOffset(Constants::NahdXml::xmlKeyStartOffset(), sample->startOffset);
            clampOffset(Constants::NahdXml::xmlKeyEndOffset(), sample->endOffset);
            clampOffset(Constants::NahdXml::xmlKeyLoopStart(), sample->loopStart);
            syncSampleFields(*sample);
        }

        stopVoicesUsing(m_samples.at(note).get());
        m_samples.at(note) = std::move(sample);
    }

    emit dataChanged();
}

std::unique_ptr<SamplerDevice::Sample> SamplerDevice::cloneSample(const Sample & source) const
{
    // The sample data is immutable, so the clone shares the buffer instead of re-reading the file.
    auto clone = std::make_unique<Sample>(source);
    // The implicit copy shares the source's insert rack, which would leave the two samples running
    // through one stateful chain. Give the clone a rack of its own.
    if (source.effectRack) {
        clone->effectRack = std::make_shared<EffectRack>();
        clone->effectRack->copyFrom(*source.effectRack);
    }
    return clone;
}

void SamplerDevice::copySample(uint8_t sourceNote, uint8_t targetNote)
{
    if (sourceNote >= maxSamples || targetNote >= maxSamples || sourceNote == targetNote) {
        return;
    }

    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        const auto & source = m_samples.at(sourceNote);
        if (!source) {
            return;
        }

        auto copy = cloneSample(*source);

        stopVoicesUsing(m_samples.at(targetNote).get());
        m_samples.at(targetNote) = std::move(copy);
        juzzlin::L(TAG).info() << "Copied the sample of note " << static_cast<int>(sourceNote) << " to note " << static_cast<int>(targetNote);
    }

    emit dataChanged();
}

void SamplerDevice::clearSample(uint8_t note)
{
    if (note >= maxSamples) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        stopVoicesUsing(m_samples.at(note).get());
        m_samples.at(note) = nullptr;
    }
    emit dataChanged();
}

const SamplerDevice::Sample * SamplerDevice::sample(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples) {
        return nullptr;
    }
    return m_samples.at(note).get();
}

std::string SamplerDevice::absoluteFilePath(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return "";
    }

    const auto filePath = QString::fromStdString(m_samples.at(note)->filePath);
    const auto resolvedPath = m_pathResolver ? m_pathResolver(filePath) : filePath;

    return (QFileInfo { resolvedPath }.isRelative() && !m_projectPath.empty())
      ? QFileInfo { QDir { QString::fromStdString(m_projectPath) }, resolvedPath }.absoluteFilePath().toStdString()
      : resolvedPath.toStdString();
}

float SamplerDevice::samplePan(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.5f;
    }
    return m_samples.at(note)->pan;
}

void SamplerDevice::setSamplePan(uint8_t note, float pan)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
                p->get().setValue(pan);
                m_samples.at(note)->pan = p->get().value();
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleVolume(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->volume;
}

void SamplerDevice::setSampleVolume(uint8_t note, float volume)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyFader().toStdString()); p) {
                p->get().setValue(volume);
                m_samples.at(note)->volume = p->get().value();
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleCutoff(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 1.0f;
    }
    return m_samples.at(note)->cutoff;
}

void SamplerDevice::setSampleCutoff(uint8_t note, float cutoff)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) {
                p->get().setValue(cutoff);
                m_samples.at(note)->cutoff = p->get().value();
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

float SamplerDevice::sampleHpfCutoff(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0f;
    }
    return m_samples.at(note)->hpfCutoff;
}

void SamplerDevice::setSampleHpfCutoff(uint8_t note, float cutoff)
{
    if (note >= maxSamples) {
        return;
    }
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (m_samples.at(note)) {
            if (auto p = m_samples.at(note)->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
                p->get().setValue(cutoff);
                m_samples.at(note)->hpfCutoff = p->get().value();
            }
            for (auto && voice : m_voices) {
                if (voice.active && voice.note == note) {
                    updateVoiceEffects(voice);
                }
            }
            changed = true;
        }
    }
    if (changed) {
        emit dataChanged();
    }
}

double SamplerDevice::sampleStartOffset(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0;
    }
    return m_samples.at(note)->startOffset;
}

void SamplerDevice::setSampleStartOffset(uint8_t note, double offset)
{
    setOffsetParameter(note, Constants::NahdXml::xmlKeyStartOffset().toStdString(), offset);
}

void SamplerDevice::setOffsetParameter(uint8_t note, const std::string & parameterName, double offset)
{
    if (note >= maxSamples) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        auto & slot = m_samples.at(note);
        if (!slot) {
            return;
        }
        // Neither trim can reach past the end of the sample it is trimming. The pair can still meet in
        // the middle, which playRange() reads as a pad with nothing left to play.
        const double duration = sampleDurationOf(*slot);
        if (auto p = slot->parameter(parameterName); p) {
            p->get().setValue(static_cast<float>(std::clamp(offset, 0.0, duration) / 60.0));
        }
        syncSampleFields(*slot);
    }
    emit dataChanged();
}

double SamplerDevice::sampleDurationOf(const Sample & sample)
{
    if (!sample.data || sample.channels <= 0 || sample.sampleRate <= 0) {
        return 0.0;
    }
    return static_cast<double>(sample.data->size() / static_cast<size_t>(sample.channels)) / static_cast<double>(sample.sampleRate);
}

double SamplerDevice::sampleEndOffset(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0;
    }
    return m_samples.at(note)->endOffset;
}

void SamplerDevice::setSampleEndOffset(uint8_t note, double offset)
{
    setOffsetParameter(note, Constants::NahdXml::xmlKeyEndOffset().toStdString(), offset);
}

double SamplerDevice::sampleLoopStart(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0.0;
    }
    return m_samples.at(note)->loopStart;
}

void SamplerDevice::setSampleLoopStart(uint8_t note, double offset)
{
    setOffsetParameter(note, Constants::NahdXml::xmlKeyLoopStart().toStdString(), offset);
}

float SamplerDevice::sampleTune(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeyTune().toStdString(), 0.5f);
}

void SamplerDevice::setSampleTune(uint8_t note, float tune)
{
    // Coarse tuning moves in whole semitones, so snap the position to one: the knob then reads out the
    // semitone it will actually play rather than the place the pointer happened to land.
    const float snapped = 0.5f + static_cast<float>(tuneSemitones(tune)) / (2.0f * TuneSemitoneRange);
    setPadValue(note, Constants::NahdXml::xmlKeyTune().toStdString(), snapped);
}

float SamplerDevice::sampleDetune(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeyDetune().toStdString(), 0.5f);
}

void SamplerDevice::setSampleDetune(uint8_t note, float detune)
{
    setPadValue(note, Constants::NahdXml::xmlKeyDetune().toStdString(), detune);
}

float SamplerDevice::sampleAttack(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f);
}

void SamplerDevice::setSampleAttack(uint8_t note, float attack)
{
    setPadValue(note, Constants::NahdXml::xmlKeyAttack().toStdString(), attack);
}

float SamplerDevice::sampleDecay(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeyDecay().toStdString(), 0.0f);
}

void SamplerDevice::setSampleDecay(uint8_t note, float decay)
{
    setPadValue(note, Constants::NahdXml::xmlKeyDecay().toStdString(), decay);
}

float SamplerDevice::sampleSustain(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeySustain().toStdString(), 1.0f);
}

void SamplerDevice::setSampleSustain(uint8_t note, float sustain)
{
    setPadValue(note, Constants::NahdXml::xmlKeySustain().toStdString(), sustain);
}

float SamplerDevice::sampleRelease(uint8_t note) const
{
    return padValue(note, Constants::NahdXml::xmlKeyReleaseTime().toStdString(), 0.0f);
}

void SamplerDevice::setSampleRelease(uint8_t note, float release)
{
    setPadValue(note, Constants::NahdXml::xmlKeyReleaseTime().toStdString(), release);
}

bool SamplerDevice::sampleReverse(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return false;
    }
    return m_samples.at(note)->reverse;
}

void SamplerDevice::setSampleReverse(uint8_t note, bool reverse)
{
    setPadValue(note, Constants::NahdXml::xmlKeyReverse().toStdString(), reverse ? 1.0f : 0.0f);
}

bool SamplerDevice::sampleLoop(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return false;
    }
    return m_samples.at(note)->loop;
}

void SamplerDevice::setSampleLoop(uint8_t note, bool loop)
{
    setPadValue(note, Constants::NahdXml::xmlKeyLoop().toStdString(), loop ? 1.0f : 0.0f);
}

int SamplerDevice::sampleChokeGroup(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return 0;
    }
    return m_samples.at(note)->chokeGroup;
}

void SamplerDevice::setSampleChokeGroup(uint8_t note, int group)
{
    setPadValue(note, Constants::NahdXml::xmlKeyChokeGroup().toStdString(), static_cast<float>(std::clamp(group, 0, maxChokeGroup)));
}

double SamplerDevice::sampleDuration(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note) || !m_samples.at(note)->data) {
        return 0.0;
    }
    const auto & s = m_samples.at(note);
    return static_cast<double>(s->data->size() / static_cast<size_t>(s->channels)) / static_cast<double>(s->sampleRate);
}

EffectRack & SamplerDevice::sampleEffectRack(uint8_t note)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    auto & slot = m_samples.at(note);
    if (!slot) {
        slot = std::make_unique<Sample>();
    }
    if (!slot->effectRack) {
        slot->effectRack = std::make_shared<EffectRack>();
    }
    return *slot->effectRack;
}

bool SamplerDevice::channelMode() const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_channelMode;
}

void SamplerDevice::setChannelMode(bool enabled)
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
            p->get().setValue(enabled ? 1.0f : 0.0f);
            m_channelMode = p->get().value() > 0.5f;
        }
    }
    emit dataChanged();
}

bool SamplerDevice::chromaticMode() const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_chromaticMode;
}

void SamplerDevice::setChromaticMode(bool enabled)
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyChromaticMode().toStdString()); p) {
            p->get().setValue(enabled ? 1.0f : 0.0f);
            m_chromaticMode = p->get().value() > 0.5f;
        }
    }
    emit dataChanged();
}

const SamplerDevice::Sample * SamplerDevice::coveringSample(uint8_t note, uint8_t & rootNote) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };

    // Find the greatest octave root (multiple of 12) at or below the note that has a sample. If there is none
    // below, fall back to the lowest set root so the lowest sample also covers everything beneath it.
    const Sample * covering = nullptr;
    rootNote = 0;

    for (int root = (note / 12) * 12; root >= 0; root -= 12) {
        if (m_samples.at(static_cast<size_t>(root))) {
            covering = m_samples.at(static_cast<size_t>(root)).get();
            rootNote = static_cast<uint8_t>(root);
            return covering;
        }
    }

    // Nothing at or below: use the lowest set root above the note.
    for (int root = ((note / 12) + 1) * 12; root < static_cast<int>(maxSamples); root += 12) {
        if (m_samples.at(static_cast<size_t>(root))) {
            covering = m_samples.at(static_cast<size_t>(root)).get();
            rootNote = static_cast<uint8_t>(root);
            return covering;
        }
    }

    return nullptr;
}

double SamplerDevice::chromaticPitchRatio(uint8_t note) const
{
    uint8_t rootNote = 0;
    if (!coveringSample(note, rootNote)) {
        return 1.0;
    }
    return std::pow(2.0, (static_cast<double>(note) - static_cast<double>(rootNote)) / 12.0);
}

bool SamplerDevice::embedWaveData() const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_embedWaveData;
}

void SamplerDevice::setEmbedWaveData(bool enabled)
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        if (auto p = parameter(Constants::NahdXml::xmlKeyEmbedWaveData().toStdString()); p) {
            p->get().setValue(enabled ? 1.0f : 0.0f);
            m_embedWaveData = p->get().value() > 0.5f;
        }
    }
    emit dataChanged();
}

std::map<QString, QString> SamplerDevice::getFilesToEmbed() const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    std::map<QString, QString> files;
    if (!m_embedWaveData) {
        return files;
    }

    for (uint8_t note = 0; note < maxSamples; note++) {
        if (m_samples.at(note)) {
            const auto realPath = QString::fromStdString(absoluteFilePath(note));
            const auto nahdPath = Constants::NahdXml::embeddedDataPathPrefix() + QFileInfo { realPath }.fileName();
            files[nahdPath] = realPath;
        }
    }
    return files;
}

double SamplerDevice::playbackPosition(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto const & voice : m_voices) {
        if (voice.active && voice.note == note && voice.sample && voice.sample->data) {
            const size_t totalFrames = voice.sample->data->size() / static_cast<size_t>(voice.sample->channels);
            if (totalFrames > 0) {
                return voice.position / static_cast<double>(totalFrames);
            }
        }
    }
    return 0.0;
}

bool SamplerDevice::isFinished(uint8_t note) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto const & voice : m_voices) {
        if (voice.active && voice.note == note) {
            return false;
        }
    }
    return true;
}

void SamplerDevice::serializeToXml(ProjectWriter & writer) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeySamples());
    for (uint8_t note = 0; note < maxSamples; note++) {
        if (const auto & s = m_samples.at(note)) {
            // Skip phantom shells created solely to host an insert rack for an empty pad.
            if (s->filePath.empty() && !s->data) {
                continue;
            }
            writer.writeStartElement(Constants::NahdXml::xmlKeySample());
            writer.writeAttribute(Constants::NahdXml::xmlKeyNote(), QString::number(note));

            const auto path = [this, &s]() {
                const auto p = QString::fromStdString(s->filePath);
                if (m_embedWaveData) {
                    return Constants::NahdXml::embeddedDataPathPrefix() + QFileInfo { p }.fileName();
                }
                if (!m_projectPath.empty() && QFileInfo { p }.isAbsolute()) {
                    return QDir { QString::fromStdString(m_projectPath) }.relativeFilePath(p);
                }
                return p;
            }();

            writer.writeAttribute(Constants::NahdXml::xmlKeySamplePath(), path);

            s->serializeParametersToXml(writer);

            if (s->effectRack && s->effectRack->hasEffects()) {
                writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
                s->effectRack->serializeEffectsToXml(writer);
                writer.writeEndElement();
            }

            writer.writeEndElement();
        }
    }
    writer.writeEndElement(); // Samples
    writer.writeEndElement(); // Device
}

void SamplerDevice::deserializeFromXml(ProjectReader & reader)
{
    deserializeAttributesFromXml(reader);

    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyParameters()) {
            deserializeParametersFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyInsertEffects()) {
            insertEffectRack().deserializeEffectsFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyParameter()) {
            deserializeParameter(reader);
        } else if (name == Constants::NahdXml::xmlKeySamples()) {
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeySample()) {
                    const auto note = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
                    const auto path = reader.attribute(Constants::NahdXml::xmlKeySamplePath()).toString();
                    if (note.has_value()) {
                        loadSample(static_cast<uint8_t>(note.value()), path.toStdString());
                        std::lock_guard<std::recursive_mutex> lock { mutex() };
                        if (const auto s = m_samples.at(note.value()).get(); s) {
                            // Manual dispatch so the nested per-pad InsertEffects rack is read rather than
                            // swallowed by ParameterContainer::deserializeParametersFromXml.
                            while (reader.readNextStartElement()) {
                                if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                                    s->deserializeParameter(reader);
                                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                                    if (!s->effectRack) {
                                        s->effectRack = std::make_shared<EffectRack>();
                                    }
                                    s->effectRack->deserializeEffectsFromXml(reader);
                                } else {
                                    reader.skipCurrentElement();
                                }
                            }
                            // Sync internal fields from parameters
                            syncSampleFields(*s);
                        }
                    }
                    if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeySample()) {
                        reader.skipCurrentElement();
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        } else {
            reader.skipCurrentElement();
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        // Sync global fields
        syncParameters();
    }

    emit dataChanged();
}

void SamplerDevice::saveState()
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::saveState();
    for (size_t i = 0; i < maxSamples; i++) {
        m_savedSamples.at(i) = m_samples.at(i) ? cloneSample(*m_samples.at(i)) : nullptr;
    }
}

void SamplerDevice::restoreState()
{
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        // Cancelling the dialog throws away every sample it has been editing, so nothing may still
        // be playing through one of them.
        stopVoicesUsing(nullptr);
        for (size_t i = 0; i < maxSamples; i++) {
            m_samples.at(i) = std::move(m_savedSamples.at(i));
            m_savedSamples.at(i) = nullptr;
        }
    }
    // Emits dataChanged() of its own, which is what tells the open dialog to re-read everything
    Device::restoreState();
}

float SamplerDevice::padValue(uint8_t note, const std::string & parameterName, float fallback) const
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    if (note >= maxSamples || !m_samples.at(note)) {
        return fallback;
    }
    if (auto p = m_samples.at(note)->parameter(parameterName); p) {
        return p->get().value();
    }
    return fallback;
}

void SamplerDevice::setPadValue(uint8_t note, const std::string & parameterName, float value)
{
    if (note >= maxSamples) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> lock { mutex() };
        auto & slot = m_samples.at(note);
        if (!slot) {
            return;
        }
        if (auto p = slot->parameter(parameterName); p) {
            p->get().setValue(value);
        }
        // Tuning, trimming, reversing and the envelope are all read off the pad by the voices on every
        // callback, so syncing the pad's own fields is the whole of the update.
        syncSampleFields(*slot);
    }
    emit dataChanged();
}

void SamplerDevice::syncSampleFields(Sample & sample)
{
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p)
        sample.pan = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyFader().toStdString()); p)
        sample.volume = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p)
        sample.cutoff = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p)
        sample.hpfCutoff = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyStartOffset().toStdString()); p)
        sample.startOffset = static_cast<double>(p->get().value()) * 60.0;
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyEndOffset().toStdString()); p)
        sample.endOffset = static_cast<double>(p->get().value()) * 60.0;
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyTune().toStdString()); p)
        sample.tune = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyDetune().toStdString()); p)
        sample.detune = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p)
        sample.attack = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p)
        sample.decay = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeySustain().toStdString()); p)
        sample.sustain = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyReleaseTime().toStdString()); p)
        sample.release = p->get().value();
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyReverse().toStdString()); p)
        sample.reverse = p->get().value() > 0.5f;
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyLoop().toStdString()); p)
        sample.loop = p->get().value() > 0.5f;
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyLoopStart().toStdString()); p)
        sample.loopStart = static_cast<double>(p->get().value()) * 60.0;
    if (auto p = sample.parameter(Constants::NahdXml::xmlKeyChokeGroup().toStdString()); p)
        sample.chokeGroup = static_cast<int>(std::lround(p->get().value()));
}

bool SamplerDevice::clearAutomationInternal()
{
    bool changed = Device::clearAutomationInternal();

    for (auto && sample : m_samples) {
        if (sample && sample->isAutomated()) {
            sample->clearAutomation();
            syncSampleFields(*sample);
            changed = true;
        }
    }

    // The global cutoffs ride MIDI CC without ever reaching a parameter, so they keep a pair of
    // their own. Nothing but CC writes them, which makes the defaults their authored values.
    if (std::abs(m_globalCutoff - m_authoredGlobalCutoff) > 0.0001f || std::abs(m_globalHpfCutoff - m_authoredGlobalHpfCutoff) > 0.0001f) {
        m_globalCutoff = m_authoredGlobalCutoff;
        m_globalHpfCutoff = m_authoredGlobalHpfCutoff;
        changed = true;
    }

    if (changed) {
        for (auto && voice : m_voices) {
            if (voice.active && voice.sample) {
                voice.pan = panInternal();
                voice.cutoff = m_globalCutoff;
                voice.hpfCutoff = m_globalHpfCutoff;
                updateVoiceEffects(voice);
            }
        }
    }

    return changed;
}

void SamplerDevice::setProjectPath(const std::string & projectPath)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_projectPath = projectPath;
}

void SamplerDevice::setPathResolver(PathResolver resolver)
{
    std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_pathResolver = std::move(resolver);
}

void SamplerDevice::setPan(float pan)
{
    Device::setPan(pan);
}

void SamplerDevice::setVolume(float volume)
{
    Device::setVolume(volume);
}

float SamplerDevice::gain() const
{
    return Device::gain();
}

void SamplerDevice::setGain(float gain)
{
    Device::setGain(gain);
}

void SamplerDevice::syncParameters()
{
    Device::syncParameters();
    if (auto p = parameter(Constants::NahdXml::xmlKeyChannelMode().toStdString()); p) {
        m_channelMode = p->get().value() > 0.5f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyChromaticMode().toStdString()); p) {
        m_chromaticMode = p->get().value() > 0.5f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyEmbedWaveData().toStdString()); p) {
        m_embedWaveData = p->get().value() > 0.5f;
    }

    // Update active voices with new global parameters
    for (auto && voice : m_voices) {
        if (voice.active) {
            voice.pan = panInternal();
            voice.cutoff = m_globalCutoff;
            voice.hpfCutoff = m_globalHpfCutoff;
            updateVoiceEffects(voice);
        }
    }
}

} // namespace noteahead
