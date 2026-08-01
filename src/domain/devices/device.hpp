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

    //! Fader position a MIDI Channel Volume (CC 7) value maps to. Full CC lands on unity rather
    //! than on the top of the throw, so CC 7 keeps meaning what it always did.
    static float faderPositionFromMidiCc(uint8_t value);

    float gain() const;
    virtual void setGain(float gain);

    float pan() const;
    virtual void setPan(float pan);

    float reverbSend(size_t index) const;
    virtual void setReverbSend(size_t index, float send);
    size_t reverbSendCount() const;

signals:
    void dataChanged();
    void sampleRateChanged();

protected:
    void serializeAttributesToXml(ProjectWriter & writer) const;
    void deserializeAttributesFromXml(ProjectReader & reader);

    virtual void syncParameters();

    void setContinuousParameterValue(const std::string & key, float value);
    void setDiscreteParameterValue(const std::string & key, int value);

    bool updateVolumeParameter(float volume, bool updateManual);
    bool updateGainParameter(float gain, bool updateManual);
    bool updatePanParameter(float pan, bool updateManual);
    bool updateReverbSendParameter(size_t index, float send, bool updateManual);

    std::recursive_mutex & mutex() const;

    float volumeInternal() const;
    float gainInternal() const;
    float panInternal() const;
    float reverbSendInternal(size_t index) const;
    float linearGainInternal() const;
    float manualVolumeInternal() const;
    float manualGainInternal() const;
    float manualPanInternal() const;
    float manualReverbSendInternal(size_t index) const;
    void setManualVolume(float volume);
    void setManualGain(float gain);
    void setManualPan(float pan);
    void setManualReverbSend(size_t index, float send);

private:
    size_t m_id { 0 };
    uint32_t m_sampleRate { static_cast<uint32_t>(Constants::defaultSampleRate()) };

    //! Fader position, seeded to unity in the constructor.
    float m_volume { 1.0f };
    float m_gain { 0.5f };
    float m_pan { 0.5f };
    std::vector<float> m_reverbSends;
    float m_linearGain { 1.0f };

    // Manual settings for CC reset
    float m_manualVolume { 1.0f };
    float m_manualGain { 0.5f };
    float m_manualPan { 0.5f };
    std::vector<float> m_manualReverbSends;
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
