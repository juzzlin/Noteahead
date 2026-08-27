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

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <QObject>

#include "../dsp/audio_context.hpp"
#include "../effects/effect_rack.hpp"
#include "../tracker/parameter_container.hpp"
#include "../utility/audio_scope.hpp"
#include "../utility/clip_detector.hpp"
#include "../utility/level_meter.hpp"
#include "../utility/load_meter.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

namespace Constants {
double defaultSampleRate();
}

struct MidiCcController
{
    uint8_t number;
    std::string name;
    int minValue { 0 };
    int maxValue { 127 };
    //! MIDI note this controller belongs to, when it drives one key or pad rather than the whole
    //! device. Naming the note is left to the presentation layer, which is where note names live.
    std::optional<uint8_t> note {};
};

class Device : public QObject, public ParameterContainer
{
    Q_OBJECT

public:
    //! Where the fader sits relative to the insert effect rack.
    //!
    //! PreInserts is how the device behaved before the channel strip existed and stays the default,
    //! so projects saved back then load unchanged. PostInserts is the gain-staging arrangement: Gain
    //! trims the source into the inserts and the fader only balances what comes out of them, so
    //! riding it can no longer change how hard a compressor is driven.
    enum class FaderPosition
    {
        PreInserts = 0,
        PostInserts = 1
    };

    //! Where the effect sends tap the signal.
    enum class SendTap
    {
        PostFader = 0,
        PreFader = 1
    };

    Device();
    virtual ~Device() override = default;

    virtual std::string name() const = 0;
    virtual std::string category() const = 0;
    virtual std::string typeName() const = 0;
    virtual std::string typeId() const = 0;

    virtual std::vector<MidiCcController> availableMidiCcControllers() const;

    size_t id() const;
    void setId(size_t id);

    virtual void processMidiNoteOn(uint8_t note, uint8_t velocity) = 0;
    virtual void processMidiNoteOff(uint8_t note) = 0;
    virtual void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) = 0;

    virtual void processMidiPitchBend(uint16_t /*value*/, uint8_t /*channel*/)
    {
    }

    virtual void processMidiProgramChange(uint8_t, uint8_t)
    {
    }

    virtual void processMidiAllNotesOff() = 0;

    virtual void processAudio(AudioContext & context) = 0;
    void processInsertEffects(AudioContext & context);
    //! Applies the fader in place over the whole buffer.
    //!
    //! Devices deliberately do not do this themselves: the engine calls it either side of the
    //! insert rack depending on faderPosition(), which is the whole point of the setting.
    //!
    //! Master pan is *not* part of this and stays inside each device. Synth, Wavetable Synth and
    //! Sampler fold it into per-voice panning — clamped together with voice spread and pan
    //! modulation, or as the seed a per-pad pan overrides — so lifting it out would double-apply
    //! it there and silently change existing patches.
    void applyFader(AudioContext & context) const;

    FaderPosition faderPosition() const;
    void setFaderPosition(FaderPosition position);

    SendTap sendTap() const;
    void setSendTap(SendTap tap);
    //! Device slots whose direct contribution to the master this device takes over.
    //!
    //! A device that mixes other devices' outputs lists them here so the engine can suppress
    //! their direct path; they are then heard only through this device. Keeps the engine free of
    //! any knowledge of which concrete device types do the mixing.
    virtual std::vector<size_t> claimedOutputSlots() const;

    virtual std::vector<size_t> sidechainDependencies() const;
    //! Allocation-free variant used on the audio thread; fills out (cleared first).
    virtual void sidechainDependencies(std::vector<size_t> & out) const;
    EffectRack & insertEffectRack();
    const EffectRack & insertEffectRack() const;

    //! Takes over another device's channel strip: insert effects, sends, fader, gain, pan, fader
    //! position and send tap. Everything on a slot that is not the instrument itself.
    //!
    //! What makes swapping the instrument in a slot keep the mix around it. Lives here rather than
    //! in the service because none of it belongs to a concrete device type, so any device can
    //! adopt the strip of any other.
    void adoptChannelStripFrom(const Device & other);

    //! Oscilloscope tap for this device's output. Capture is gated by AudioScope::setActive().
    AudioScope & scope();

    //! Level tap taken post-gain and pre-insert, which is the level Gain is set against. Unlike the
    //! other taps its meaning does not move when faderPosition() changes. Gated by setActive().
    LevelMeter & meter();
    const LevelMeter & meter() const;

    //! Share of the audio buffer's real-time budget this device's processing takes. Gated by
    //! setActive() like the other taps.
    LoadMeter & loadMeter();
    const LoadMeter & loadMeter() const;

    //! Full-scale latch on this device's final output, taken after both the fader and the insert
    //! rack. Ungated, unlike the meters, so it still catches clipping with no dialog open.
    ClipDetector & clipDetector();
    const ClipDetector & clipDetector() const;

    virtual bool hasActiveAudio() const
    {
        return true;
    }

    virtual void setBpm(float bpm);

    virtual void reset() override;
    virtual void resetAudio();

    //! Remembers the current settings so restoreState() can put them back. Taken when a device
    //! dialog opens; its Cancel button is what calls restoreState(). Authored values only, so that
    //! opening a dialog over a playing song cannot bake its automation into the project.
    virtual void saveState();
    virtual void restoreState();

    //! Puts every automated parameter back to the value the user authored, and re-syncs the DSP.
    //!
    //! MIDI CC writes only the live layer, so this is how the transport hands the device back after
    //! playback: the sound and the knobs return to the patch, and nothing that was played is left
    //! behind to be saved.
    void clearAutomation();

    uint32_t sampleRate() const;
    void setSampleRate(uint32_t sampleRate);

    virtual void serializeToXml(ProjectWriter & writer) const;
    virtual void deserializeFromXml(ProjectReader & reader);

    //! The fader parameter shared by every device and by the Sampler's pads. Public so the pads,
    //! which are not Devices, get the same taper and the same legacy conversion.
    static Parameter faderParameter();

    //! Position on the fader throw, 0..1, not a gain: run it through ParameterMapper::mapFader().
    float volume() const;
    virtual void setVolume(float volume);

    //! Fader position a MIDI Channel Volume (CC 7) value maps to. 127 lands on unity rather than on
    //! the top of the throw, so CC 7 keeps meaning what it always did, and the values above it
    //! reach into the boost range.
    static float faderPositionFromMidiCc(uint8_t value);

    //! CC 7 as an internal device exposes it, carrying the extended value range.
    static MidiCcController faderMidiCcController();

    float gain() const;
    virtual void setGain(float gain);

    float pan() const;
    virtual void setPan(float pan);

    float reverbSend(size_t index) const;
    virtual void setReverbSend(size_t index, float send);
    size_t reverbSendCount() const;

signals:
    //! The device itself changed in a way the rest of the application has to react to: it was
    //! added, removed, renamed, re-patched. Deliberately *not* emitted for incoming MIDI CC.
    void dataChanged();
    //! A parameter value moved. Emitted for MIDI CC traffic, which during playback of an automation
    //! arrives many times per beat, so only the device's own dialog listens to it. Routing that
    //! through dataChanged() reset the Device Rack model and rebuilt the port lists per event.
    void parametersChanged();
    void sampleRateChanged();

protected:
    void serializeAttributesToXml(ProjectWriter & writer) const;
    void deserializeAttributesFromXml(ProjectReader & reader);

    virtual void syncParameters();

    //! Clears automation without emitting, for callers that already hold the device mutex -- the
    //! MIDI CC handlers, which report the change themselves once the lock is gone. Returns whether
    //! anything actually moved. Devices with transient state of their own extend this.
    virtual bool clearAutomationInternal();

    void setContinuousParameterValue(const std::string & key, float value);
    void setDiscreteParameterValue(const std::string & key, int value);

    //! \param authored Whether the value is the user's (writes the document) or automation's
    //! (live only). Everything the transport generates passes false.
    bool updateVolumeParameter(float volume, bool authored);
    bool updateGainParameter(float gain, bool authored);
    bool updatePanParameter(float pan, bool authored);
    //! Reverb sends are not parameters and nothing automates them, so they have no live layer.
    bool updateReverbSendParameter(size_t index, float send);

    std::recursive_mutex & mutex() const;

    float volumeInternal() const;
    float gainInternal() const;
    float panInternal() const;
    float reverbSendInternal(size_t index) const;
    float linearGainInternal() const;

private:
    size_t m_id { 0 };
    uint32_t m_sampleRate { static_cast<uint32_t>(Constants::defaultSampleRate()) };

    //! Fader position, seeded to unity in the constructor.
    float m_volume { 1.0f };
    float m_gain { 0.5f };
    float m_pan { 0.5f };
    std::vector<float> m_reverbSends;
    float m_linearGain { 1.0f };

    //! Settings as they were when a device dialog opened; restoreState() puts these back.
    ParameterSnapshot m_savedParameters;

    // Manual settings for CC reset
    FaderPosition m_faderPosition { FaderPosition::PreInserts };
    SendTap m_sendTap { SendTap::PostFader };
    EffectRack m_insertEffectRack;
    AudioScope m_scope;
    LevelMeter m_meter;
    LoadMeter m_loadMeter;
    ClipDetector m_clipDetector;

    mutable std::recursive_mutex m_mutex;
};

} // namespace noteahead

#endif // DEVICE_HPP
