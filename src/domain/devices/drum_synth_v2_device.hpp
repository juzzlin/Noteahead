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

#ifndef DRUM_SYNTH_V2_DEVICE_HPP
#define DRUM_SYNTH_V2_DEVICE_HPP

#include "../dsp/drum/crash_engine.hpp"
#include "../dsp/drum/drum_amp_envelope.hpp"
#include "../dsp/drum/hihat_engine.hpp"
#include "../dsp/drum/kick_engine.hpp"
#include "../dsp/drum/ride_engine.hpp"
#include "../dsp/drum/snare_engine.hpp"
#include "../dsp/drum/tom_engine.hpp"
#include "../dsp/high_pass_filter.hpp"
#include "../dsp/low_pass_filter.hpp"
#include "../dsp/panning.hpp"
#include "../dsp/upsampler.hpp"
#include "../dsp/volume.hpp"
#include "device.hpp"
#include "drum_synth_v2_constants.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace noteahead {

class DrumSynthV2Device : public Device
{
public:
    explicit DrumSynthV2Device(std::string name);
    virtual ~DrumSynthV2Device() override = default;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
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

protected:
    void syncParameters() override;

private:
    bool writeVoiceParameter(int voiceIndex, const std::string & paramName, float value, bool authored);

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
    void addVoiceParameters(int index);
    void addAmpEnvelopeParameters(int index, const std::string & prefix);
    //! See the definition: what a kit saved before the sustain stage was voiced against.
    void restoreLegacyAmpEnvelope();
    void addKickParameters(const std::string & prefix);
    void addSnareParameters(const std::string & prefix);
    void addTomParameters(const std::string & prefix);
    void addHiHatParameters(const std::string & prefix);
    void addCymbalParameters(const std::string & prefix);

    void syncVoiceParameters(int index);
    void syncAmpEnvelopeParameters(int index, const std::string & prefix);
    void syncCommonEngineParameters(int index, const std::string & prefix);
    void syncKickParameters(const std::string & prefix);
    void syncSnareParameters(const std::string & prefix);
    void syncClapParameters(const std::string & prefix);
    void syncTomParameters(int index, const std::string & prefix);
    void syncHiHatParameters(int index, const std::string & prefix);
    void syncCymbalParameters(int index, const std::string & prefix);
};

} // namespace noteahead

#endif // DRUM_SYNTH_V2_DEVICE_HPP
