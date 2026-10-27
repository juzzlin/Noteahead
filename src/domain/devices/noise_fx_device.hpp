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

#ifndef NOISE_FX_DEVICE_HPP
#define NOISE_FX_DEVICE_HPP

#include "../dsp/adsr_envelope.hpp"
#include "../dsp/lfo.hpp"
#include "../dsp/noise_source.hpp"
#include "../dsp/poly_blep_oscillator.hpp"
#include "../dsp/saturating_svf.hpp"
#include "../dsp/sweep_envelope.hpp"
#include "../dsp/true_stereo_panner.hpp"
#include "device.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! Noise and sound-effect generator: risers, downlifters, sweeps, wind, crackle, zaps, sirens.
//!
//! Each voice blends stereo coloured noise with a tone oscillator, which can be ring modulated by
//! the noise, adds crackle, and runs that through a resonant, saturating filter per channel, a
//! tempo-synced gate and an amp envelope. Two sweeps ramp from a start to an end value over seconds
//! or beats, and an LFO cycles; each of the three drives two destinations. Synced to beats, one note
//! fires a riser that ends on the downbeat it was aimed at.
//!
//! With Play to End on, a note ignores its note-off and is released when Sweep A has run its course,
//! note-off or not, which is what makes a single tracker note a whole riser. A repeating Sweep A
//! never ends, so there the note-off releases the note as usual.
class NoiseFxDevice : public Device
{
public:
    static constexpr size_t VoiceCount { 4 };
    static constexpr size_t SweepCount { 2 };
    static constexpr size_t DestinationCount { 2 };

    //! Append only: persisted as the ordinal.
    enum class FilterType
    {
        LowPass,
        BandPass,
        HighPass,
        Notch
    };
    static constexpr int FilterTypeCount { 4 };

    //! Append only: persisted as the ordinal.
    enum class Destination
    {
        None,
        Cutoff,
        Resonance,
        Colour,
        SampleHoldRate,
        Level,
        Pan,
        Pitch,
        Blend,
        LfoRate,
        LfoDepth,
        GateRate
    };
    static constexpr int DestinationTypeCount { 12 };

    //! The LFO's shapes are Lfo::Waveform's ordinals, and then this one after them: random values
    //! glided into one another over each cycle, rather than stepped as Random is.
    static constexpr int SmoothRandomWaveform { 5 };

    //! The synced sweep lengths, in beats: 1/16 bar to 32 bars.
    static const std::vector<double> & syncLengthBeats();
    //! The gate's cycle lengths, in beats: a bar down to 1/64.
    static const std::vector<double> & gateDivisionBeats();
    //! The synced LFO's cycle lengths, in bars: 4 bars down to 1/32.
    static const std::vector<double> & lfoSyncBars();

    explicit NoiseFxDevice(std::string name);
    ~NoiseFxDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();

    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;
    void processMidiProgramChange(uint8_t program, uint8_t channel) override;

    //! Loads factory preset @p index of NoiseFxPresets as the user's own settings.
    void loadPreset(int index);

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    // Noise
    float colour() const;
    void setColour(float colour);
    //! 1 is off: every sample passes.
    float sampleHoldRate() const;
    void setSampleHoldRate(float rate);
    float crackle() const;
    void setCrackle(float crackle);
    //! The noise's level in the mix, on a squared curve; the crackle is left as it is.
    float noiseLevel() const;
    void setNoiseLevel(float level);
    //! 0 plays the same noise on both channels, 1 uncorrelated noise.
    float width() const;
    void setWidth(float width);

    // Tone
    PolyBlepOscillator::Waveform toneWaveform() const;
    void setToneWaveform(PolyBlepOscillator::Waveform waveform);
    //! Bipolar, 0..1 about 0.5: two octaves either way of the note.
    float pitch() const;
    void setPitch(float pitch);
    //! 0 is all noise, 1 all tone.
    float blend() const;
    void setBlend(float blend);
    //! How much of the tone is multiplied by the noise.
    float ringMod() const;
    void setRingMod(float ringMod);

    // Filter
    FilterType filterType() const;
    void setFilterType(FilterType type);
    float cutoff() const;
    void setCutoff(float cutoff);
    float resonance() const;
    void setResonance(float resonance);
    float drive() const;
    void setDrive(float drive);
    float keyTrack() const;
    void setKeyTrack(float keyTrack);

    // Amp
    float ampAttack() const;
    void setAmpAttack(float attack);
    float ampDecay() const;
    void setAmpDecay(float decay);
    float ampSustain() const;
    void setAmpSustain(float sustain);
    float ampRelease() const;
    void setAmpRelease(float release);
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);
    bool playToEnd() const;
    void setPlayToEnd(bool playToEnd);

    //! The key sweep @p sweep's control @p key is stored under, "sweep1_start".
    static std::string sweepKey(size_t sweep, const QString & key);
    //! The key of destination @p destination of sweep @p sweep, "sweep1_target2".
    static std::string sweepDestinationKey(size_t sweep, const QString & key, size_t destination);
    //! The key the LFO's control @p key is stored under, "lfo_rate".
    static std::string lfoKey(const QString & key);
    //! The key of destination @p destination of the LFO, "lfo_target2".
    static std::string lfoDestinationKey(const QString & key, size_t destination);
    //! The key the gate's control @p key is stored under, "gate_rate".
    static std::string gateKey(const QString & key);

    //! Sweep controls by key: Start, End, Curve, Length (seconds), Sync, Sync Length (an index into
    //! syncLengthBeats()) and Mode. Discrete ones read and take their integer value.
    float sweepParameterValue(size_t sweep, const QString & key) const;
    void setSweepParameterValue(size_t sweep, const QString & key, float value);

    Destination sweepTarget(size_t sweep, size_t destination) const;
    void setSweepTarget(size_t sweep, size_t destination, Destination target);
    //! Bipolar, 0..1 about 0.5.
    float sweepDepth(size_t sweep, size_t destination) const;
    void setSweepDepth(size_t sweep, size_t destination, float depth);

    //! LFO controls by key: Waveform (Lfo::Waveform or SmoothRandomWaveform), Sync and Rate. Synced,
    //! the rate picks from lfoSyncBars().
    float lfoParameterValue(const QString & key) const;
    void setLfoParameterValue(const QString & key, float value);

    //! The LFO cannot reach its own rate or depth: those destinations do nothing for it.
    Destination lfoTarget(size_t destination) const;
    void setLfoTarget(size_t destination, Destination target);
    //! Bipolar, 0..1 about 0.5.
    float lfoDepth(size_t destination) const;
    void setLfoDepth(size_t destination, float depth);

    //! Gate controls by key: Rate (an index into gateDivisionBeats()), Depth (0 is off), Width (the
    //! open part of each cycle) and Smoothing (the edges).
    float gateParameterValue(const QString & key) const;
    void setGateParameterValue(const QString & key, float value);

protected:
    void syncParameters() override;

private:
    struct Route
    {
        std::array<Destination, DestinationCount> targets {};
        std::array<float, DestinationCount> depths { 0.5f, 0.5f };
    };

    struct SweepSettings
    {
        float start { 0.0f };
        float end { 1.0f };
        float curve { 0.5f };
        SweepEnvelope::Mode mode { SweepEnvelope::Mode::Hold };
        bool sync { true };
        int syncLength { 0 };
        float length { 0.5f };
        Route route;
    };

    struct LfoSettings
    {
        int waveform { static_cast<int>(Lfo::Waveform::Sine) };
        bool sync { false };
        float rate { 0.5f };
        Route route;
    };

    struct GateSettings
    {
        int rate { 0 };
        float depth { 0.0f };
        float width { 0.5f };
        float smoothing { 0.3f };
    };

    //! What the sweeps and the LFO add up to on one sample, as offsets to the controls they move,
    //! except for level and LFO depth, which are factors.
    struct Modulation
    {
        double cutoff { 0.0 };
        double resonance { 0.0 };
        double colour { 0.0 };
        double holdRate { 0.0 };
        double level { 1.0 };
        double pan { 0.0 };
        double pitch { 0.0 };
        double blend { 0.0 };
        double lfoRate { 0.0 };
        double lfoDepth { 1.0 };
        double gateRate { 0.0 };
    };

    struct Voice
    {
        Voice(uint32_t noiseSeed, uint32_t sideNoiseSeed, uint32_t crackleSeed);

        //! Two independent noises, mixed into left and right by the width.
        NoiseSource noise;
        NoiseSource sideNoise;
        CrackleSource crackle;
        PolyBlepOscillator tone;
        SaturatingSvf filterL;
        SaturatingSvf filterR;
        AdsrEnvelope amp;
        std::array<SweepEnvelope, SweepCount> sweeps;
        Lfo lfo;
        //! Smooth Random glides from the previous random value to the current one. Empty until the
        //! LFO has given its first value after the note-on.
        double lfoGlideFrom { 0.0 };
        std::optional<double> lfoGlideTo;
        double gatePhase { 0.0 };
        double gateLevel { 1.0 };
        uint8_t note { 0 };
        double velocityGain { 1.0 };
        bool active { false };
        //! Playing to the end of Sweep A: released when it gets there, and deaf to the note-off.
        bool releaseAtSweepEnd { false };

        void reset();
    };

    //! @p authored writes the saved settings; otherwise only the automation layer, as transport
    //! traffic must leave what the user saved alone.
    void applyPreset(int index, bool authored);

    void addSweepParameters(size_t sweep);
    void addLfoParameters();
    void addGateParameters();
    void addRouteParameters(const std::string & targetKeyBase, const std::string & depthKeyBase, Destination defaultTarget, float defaultDepth);
    void syncRoute(const std::string & targetKeyBase, const std::string & depthKeyBase, Route & route);

    float keyedParameterValue(const std::string & key) const;
    void setKeyedParameterValue(const std::string & key, float value);

    void configureSweep(SweepEnvelope & envelope, const SweepSettings & settings) const;
    void configureAmp(AdsrEnvelope & amp) const;
    size_t allocateVoice();
    void releaseVoice(Voice & voice);

    //! Adds what @p route does with @p value to @p modulation. @p value01 is the same value on 0..1,
    //! which the factors use.
    static void applyRoute(const Route & route, double value, double value01, bool isLfo, Modulation & modulation);
    double lfoFrequency(double rate, double bpm) const;
    //! The voice's LFO, -1..1, with Smooth Random gliding between the values Random steps through.
    double nextLfoValue(Voice & voice) const;
    void renderVoice(Voice & voice, double sampleRate, double bpm, double & left, double & right);

    std::string m_name;

    std::vector<Voice> m_voices;
    size_t m_nextVoiceToSteal { 0 };
    TrueStereoPanner m_voicePanner;
    TrueStereoPanner m_panner;
    double m_gateSmoothingCoefficient { 0.0 };

    float m_colour { 2.0f / 3.0f };
    float m_sampleHoldRate { 1.0f };
    float m_crackle { 0.0f };
    float m_noiseLevel { 1.0f };
    float m_width { 0.8f };
    PolyBlepOscillator::Waveform m_toneWaveform { PolyBlepOscillator::Waveform::Sine };
    float m_pitch { 0.5f };
    float m_blend { 0.0f };
    float m_ringMod { 0.0f };
    FilterType m_filterType { FilterType::LowPass };
    float m_cutoff { 0.2f };
    float m_resonance { 0.3f };
    float m_drive { 0.0f };
    float m_keyTrack { 0.0f };
    float m_ampAttack { 0.0f };
    float m_ampDecay { 0.5f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.3f };
    float m_velocitySensitivity { 0.5f };
    bool m_playToEnd { true };
    std::array<SweepSettings, SweepCount> m_sweeps {};
    LfoSettings m_lfo;
    GateSettings m_gate;
};

} // namespace noteahead

#endif // NOISE_FX_DEVICE_HPP
