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

#ifndef SYNTH_DEVICE_HPP
#define SYNTH_DEVICE_HPP

#include "device.hpp"
#include "../dsp/adsr_envelope.hpp"
#include "../dsp/cascaded_svf.hpp"
#include "../dsp/polyblep_oscillator.hpp"
#include "delay_effect.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class SynthDevice : public Device
{
public:
    enum class VoiceMode
    {
        Poly,
        Unison
    };

    enum class ModTarget
    {
        Pitch1,
        Pitch2,
        Cutoff
    };

    SynthDevice();
    ~SynthDevice() override;

    std::string name() const override;
    std::string category() const override;

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(float * output, uint32_t nFrames, uint32_t sampleRate) override;

    void setBpm(float bpm) override;

    void reset() override;

    void serializeToXml(QXmlStreamWriter & writer) const override;
    void deserializeFromXml(QXmlStreamReader & reader) override;

    // Parameter accessors (VCO1)
    PolyBLEPOscillator::Waveform vco1Waveform() const;
    void setVco1Waveform(PolyBLEPOscillator::Waveform wave);
    int vco1Octave() const;
    void setVco1Octave(int octave);
    float vco1Pitch() const;
    void setVco1Pitch(float pitch);
    float vco1Shape() const;
    void setVco1Shape(float shape);
    bool vco1Sync() const;
    void setVco1Sync(bool sync);

    // Parameter accessors (VCO2)
    PolyBLEPOscillator::Waveform vco2Waveform() const;
    void setVco2Waveform(PolyBLEPOscillator::Waveform wave);
    int vco2Octave() const;
    void setVco2Octave(int octave);
    float vco2Pitch() const;
    void setVco2Pitch(float pitch);
    float vco2Shape() const;
    void setVco2Shape(float shape);
    bool vco2Sync() const;
    void setVco2Sync(bool sync);

    // Mixer
    float mixVco1() const;
    void setMixVco1(float level);
    float mixVco2() const;
    void setMixVco2(float level);

    // Filter
    float lpfCutoff() const;
    void setLpfCutoff(float cutoff);
    float lpfResonance() const;
    void setLpfResonance(float resonance);
    float hpfCutoff() const;
    void setHpfCutoff(float cutoff);
    float filterKeyTrack() const;
    void setFilterKeyTrack(float track);

    // Amp EG
    float ampAttack() const;
    void setAmpAttack(float a);
    float ampDecay() const;
    void setAmpDecay(float d);
    float ampSustain() const;
    void setAmpSustain(float s);
    float ampRelease() const;
    void setAmpRelease(float r);

    // Mod EG
    float modAttack() const;
    void setModAttack(float a);
    float modDecay() const;
    void setModDecay(float d);
    float modInt() const;
    void setModInt(float intensity);
    ModTarget modTarget() const;
    void setModTarget(ModTarget target);

    // Voice / Global
    VoiceMode voiceMode() const;
    void setVoiceMode(VoiceMode mode);
    float voiceDepth() const;
    void setVoiceDepth(float depth);
    float portamento() const;
    void setPortamento(float p);
    float panSpread() const;
    void setPanSpread(float spread);
    float masterVolume() const;
    void setMasterVolume(float vol);

    // Delay parameters
    DelayEffect::Type delayType() const;
    void setDelayType(DelayEffect::Type type);
    float delayTime() const;
    void setDelayTime(float time);
    float delayFeedback() const;
    void setDelayFeedback(float fb);
    float delayDepth() const;
    void setDelayDepth(float depth);
    float delayMix() const;
    void setDelayMix(float mix);
    bool delaySync() const;
    void setDelaySync(bool sync);
    float delaySyncDivision() const;
    void setDelaySyncDivision(float division);

    void loadPreset(int index);

private:
    struct Voice
    {
        PolyBLEPOscillator vco1;
        PolyBLEPOscillator vco2;
        CascadedSVF lpf;
        CascadedSVF hpf;
        ADSREnvelope ampEg;
        ADSREnvelope modEg;

        uint8_t note { 0 };
        double frequency { 0.0 };
        double glideFrequency { 0.0 };
        bool active { false };
        float pan { 0.5f };

        void reset();
        void trigger(uint8_t note, double freq, float pan, bool phaseSync);
        void release();
    };

    std::array<Voice, 4> m_voices;
    mutable std::mutex m_mutex;
    int m_polyNextVoice = 0;

    // Internal parameter storage
    PolyBLEPOscillator::Waveform m_vco1Waveform { PolyBLEPOscillator::Waveform::Saw };
    int m_vco1Octave { 0 };
    float m_vco1Pitch { 0.0f };
    float m_vco1Shape { 0.0f };
    bool m_vco1Sync { false };

    PolyBLEPOscillator::Waveform m_vco2Waveform { PolyBLEPOscillator::Waveform::Saw };
    int m_vco2Octave { 0 };
    float m_vco2Pitch { 0.0f };
    float m_vco2Shape { 0.0f };
    bool m_vco2Sync { false };

    float m_mixVco1 { 1.0f };
    float m_mixVco2 { 0.0f };

    float m_lpfCutoff { 1.0f };
    float m_lpfResonance { 0.0f };
    float m_hpfCutoff { 0.0f };
    float m_filterKeyTrack { 0.0f };

    float m_ampAttack { 0.1f };
    float m_ampDecay { 0.2f };
    float m_ampSustain { 1.0f };
    float m_ampRelease { 0.2f };

    float m_modAttack { 0.1f };
    float m_modDecay { 0.2f };
    float m_modInt { 0.0f };
    ModTarget m_modTarget { ModTarget::Cutoff };

    VoiceMode m_voiceMode { VoiceMode::Poly };
    float m_voiceDepth { 0.0f };
    float m_portamento { 0.0f };
    float m_panSpread { 0.0f };
    float m_masterVolume { 1.0f };

    // Manual settings for CC reset
    float m_manualPanSpread { 0.0f };
    float m_manualMasterVolume { 1.0f };
    float m_manualLpfCutoff { 1.0f };
    float m_manualHpfCutoff { 0.0f };

    DelayEffect m_delay;
    DelayEffect::Type m_delayType { DelayEffect::Type::Stereo };
    float m_delayTime { 0.5f };
    float m_delayFeedback { 0.3f };
    float m_delayDepth { 0.5f };
    float m_delayMix { 0.0f };
    bool m_delaySync { false };
    float m_delaySyncDivision { 0.25f };

    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    double midiNoteToFreq(uint8_t note) const;
    void syncParameters();
};

} // namespace noteahead

#endif // SYNTH_DEVICE_HPP
