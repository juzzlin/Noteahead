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

#ifndef DRUM_PAD_DEVICE_HPP
#define DRUM_PAD_DEVICE_HPP

#include "../dsp/drum/drum_amp_envelope.hpp"
#include "../dsp/drum/drum_engine.hpp"
#include "../dsp/high_pass_filter.hpp"
#include "../dsp/low_pass_filter.hpp"
#include "../dsp/panning.hpp"
#include "../dsp/upsampler.hpp"
#include "../dsp/volume.hpp"
#include "device.hpp"
#include "drum_synth_v2_constants.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! A drum machine of twelve pads on the General MIDI notes, each a DrumEngine of its own.
//!
//! Everything a pad passes through on its way out lives here: its filters, amp envelope, level and
//! pan, insert rack and sends, along with the hi-hat choke, the MIDI CC map, the oversampled mix and
//! the rendering the waveform view runs on. What a subclass brings is the engines themselves and the
//! parameters that drive them -- see addEngineParameters() and syncEngineParameters().
class DrumPadDevice : public Device
{
public:
    virtual ~DrumPadDevice() override = default;

    std::string name() const override;
    std::string category() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;
    //! Every drum's described parameters, grouped by drum.
    AutomatableParameterList automatableParameters() const override;

    //! Its notes choose what plays, not at what pitch, so it has no octave
    bool supportsOctave() const override;
    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    //! 0 for 12 dB/oct, which the voice filters have always been, and 1 for 24.
    int lpfSlope() const;
    void setLpfSlope(int slope);
    int hpfSlope() const;
    void setHpfSlope(int slope);

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    int selectedVoice() const;
    void setSelectedVoice(int index);

    uint8_t voiceNote(int index) const;

    //! How far into its sound one voice is, in seconds, or nothing when it is not sounding.
    //!
    //! What the waveform view's playhead runs on. In seconds rather than as a fraction, because the
    //! device does not know how long the picture beside it spans: the envelope can end the voice
    //! well before the material does, and it is the view that knows which of those it drew.
    std::optional<double> voiceElapsedSeconds(int voiceIndex) const;

    //! Renders one voice alone, from its strike until it goes quiet, and hands the frames back.
    //!
    //! Everything the voice passes through on its way out is in it, that voice's own insert effects
    //! included, because what the picture is for is showing the drum as it actually sounds.
    //!
    //! Mutates the device, so it is meant to be called on a copy rather than on one that is
    //! playing: it resets the audio state, strikes the voice and runs the whole device forward.
    //! Deterministic, since the engines are re-seeded by that reset and a drum has no pitch to
    //! follow -- the same voice renders the same frames every time.
    //! \param maxSeconds Ceiling for a voice that never quite stops, a cymbal most of all.
    //! \return Interleaved stereo frames.
    std::vector<double> renderVoiceAlone(int voiceIndex, double sampleRate, double maxSeconds);

    // Per-voice insert effect rack.
    EffectRack & voiceEffectRack(int index);

    EffectRackList effectRacks() override;
    EffectRackOpt partEffectRack(size_t partIndex) override;

    //! Every voice is a send source of its own: @p sourceIndex is the voice.
    size_t sendSourceCount() const override;
    float sendSourceLevel(size_t sourceIndex, size_t busIndex) const override;
    void setSendSourceLevel(size_t sourceIndex, size_t busIndex, float level) override;

    //! Writes a voice parameter and announces a project edit. For the dialog, which is exactly
    //! that. MIDI CC must use automateVoiceParameter() instead -- see processDeviceMidiCc().
    bool updateVoiceParameter(int voiceIndex, const std::string & paramName, float value);
    //! What one voice's parameter reads, or zero where there is no such parameter. The counterpart
    //! of updateVoiceParameter(), which is the only way in without knowing how a voice is prefixed.
    float voiceParameterValue(int voiceIndex, const std::string & paramName) const;

    //! A fresh device of the same kind, as it is before anything is loaded into it. What a copy is
    //! made from, since a device is copied by reading what another one saved.
    virtual std::unique_ptr<DrumPadDevice> newInstance(std::string name) const = 0;

protected:
    //! Sets up everything around the engines. The subclass constructor then puts an engine on every
    //! pad with setVoiceEngine(), registers the parameters with addVoiceParameters() and syncs: those
    //! steps reach its own overrides, which a base constructor cannot.
    explicit DrumPadDevice(std::string name);

    void setVoiceEngine(int index, std::unique_ptr<DrumEngine> engine);
    DrumEngine & voiceEngine(int index);

    //! Registers one voice's parameters: the ones every pad has, then the engine's own.
    void addVoiceParameters(int index);

    //! The parameters of the engine on pad @p index, every key starting with @p prefix.
    virtual void addEngineParameters(int index, const std::string & prefix) = 0;
    //! Pushes the parameters added by addEngineParameters() into the engine on pad @p index.
    virtual void syncEngineParameters(int index, const std::string & prefix) = 0;
    //! Puts back what a project saved by an older version was voiced against, before it is read.
    //! Called with the device mutex held, so it must not emit. Does nothing by default.
    virtual void restoreLegacyParameters();

    void syncParameters() override;

    bool writeVoiceParameter(int voiceIndex, const std::string & paramName, float value, bool authored);

private:
    //! Writes a voice parameter as automation: the live value moves, the saved one does not, and
    //! nothing is emitted.
    //!
    //! Emitting is the caller's job because the device mutex is held here, and the audio callback
    //! takes the engine mutex before that same device mutex -- a dataChanged() from under the lock
    //! lets its receivers walk back into the engine and the two lock orders meet in a deadlock.
    bool automateVoiceParameter(int voiceIndex, const std::string & paramName, float value);

    struct Voice
    {
        std::unique_ptr<DrumEngine> engine;
        //! Level into each global send bus, zero by default, so a kit that has never been routed
        //! anywhere sounds exactly as it always has.
        std::vector<float> sends;
        std::shared_ptr<LowPassFilter> lpf;
        std::shared_ptr<HighPassFilter> hpf;
        //! Second stage of each filter, in the chain always and neutral unless the slope asks for it.
        std::shared_ptr<LowPassFilter> lpfStage2;
        std::shared_ptr<HighPassFilter> hpfStage2;
        bool steepLpf = false;
        bool steepHpf = false;
        //! Rides on top of whatever the engine plays: see DrumAmpEnvelope. In the VCA position,
        //! after the filters, so their resonant ring is shaped with everything else.
        DrumAmpEnvelope ampEnvelope;
        //! Output frames since the voice was last struck, for the playhead. Counted per output
        //! frame rather than per oversampled one, so it is a time whatever the quality is set to.
        size_t renderedFrames {};
        std::shared_ptr<Volume> volumeEffect;
        std::shared_ptr<Panning> panningEffect;
        EffectRack effectRack;

        uint8_t midiNote { 0 };
        float level { 1.0f };
        float pan { 0.5f };
        float lpfCutoff { 1.0f };
        float hpfCutoff { 0.0f };

        void updateEffects();
    };

    //! 0 for 12 dB/oct, which the voice filters have always been, and 1 for 24.
    float m_lpfSlope = 0.0f;
    float m_hpfSlope = 0.0f;
    std::string m_name;
    std::array<Voice, DrumSynthV2::NumVoices> m_voices;
    int m_selectedVoice { 0 };

    //! Scratch buffer for the oversampled mix, kept as a member so no allocation happens on the
    //! audio thread. It only ever grows.
    std::vector<float> m_oversampledBuffer;

    //! The same, one per send bus, for the voices routed to it. Only the buses actually used are
    //! filled and decimated, so a kit that sends nowhere pays nothing at all.
    std::vector<std::vector<float>> m_sendOversampledBuffers;
    //! One decimator pair per bus, since each carries a signal of its own.
    std::vector<std::pair<Decimator, Decimator>> m_sendDecimators;

    //! Adds the voices routed to a bus into that bus, decimating what was accumulated at the
    //! oversampled rate. See Device::sendSourceCount().
    void addSendContributions(AudioContext & context, uint8_t oversampleFactor, double panL, double panR);

    //! Per-voice snapshot of the enabled insert-rack effects, rebuilt every block but kept between
    //! blocks so that refilling it costs nothing: this runs on the audio thread, which must not
    //! allocate. m_rackEffectScratch is the staging the rack is read into.
    std::array<std::vector<EffectRack::EffectS>, DrumSynthV2::NumVoices> m_voiceRackEffects;
    std::vector<EffectRack::EffectS> m_rackEffectScratch;

    Decimator m_downsamplerL;
    Decimator m_downsamplerR;

    void initializeVoices();
    void addAmpEnvelopeParameters(int index, const std::string & prefix);

    void syncVoiceParameters(int index);
    void syncAmpEnvelopeParameters(int index, const std::string & prefix);
};

} // namespace noteahead

#endif // DRUM_PAD_DEVICE_HPP
