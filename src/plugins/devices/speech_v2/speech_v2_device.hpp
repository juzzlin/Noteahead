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

#ifndef SPEECH_V2_DEVICE_HPP
#define SPEECH_V2_DEVICE_HPP

#include "../../../domain/devices/device.hpp"
#include "../../../domain/dsp/cascaded_svf.hpp"
#include "../../../domain/dsp/dc_blocker.hpp"
#include "../../../domain/dsp/lfo.hpp"
#include "../../../domain/dsp/speech/formant_voice_v2.hpp"
#include "../../../domain/dsp/speech/speech_prosody.hpp"
#include "../../../domain/dsp/speech/speech_sequencer_v2.hpp"
#include "../../../domain/dsp/true_stereo_panner.hpp"

#include <array>
#include <optional>
#include <set>
#include <string>

namespace noteahead {

//! Second-generation formant speech synthesizer powered by FormantVoiceV2.
class SpeechV2Device : public Device
{
public:
    explicit SpeechV2Device(std::string name);
    ~SpeechV2Device() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;
    std::vector<std::string> modulationWheelTargets() const override;

    std::vector<MidiCcController> deviceMidiCcControllers() const override;

    static std::string typeIdString();
    static std::string defaultPhrase();

    void processDeviceMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processDeviceMidiNoteOff(uint8_t note) override;
    void processDeviceMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiPitchBend(uint16_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    bool wantsNoteIndexSeek() const override;
    void seekToNoteIndex(size_t noteIndex) override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;
    void advanceIdle(const AudioContext & context) override;

    void reset() override;
    void resetDeviceAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    std::string phrase() const;
    void setPhrase(const std::string & phrase);
    std::string phrasePhonemes() const;

    float rate() const;
    void setRate(float rate);
    float portamento() const;
    void setPortamento(float portamento);
    float glide() const;
    void setGlide(float glide);
    float formantShift() const;
    void setFormantShift(float formantShift);
    float breathiness() const;
    void setBreathiness(float breathiness);
    float consonantLevel() const;
    void setConsonantLevel(float consonantLevel);
    float sibilance() const;
    void setSibilance(float sibilance);
    int voiceType() const;
    void setVoiceType(int voiceType);
    float openQuotient() const;
    void setOpenQuotient(float openQuotient);
    float voicePerturbation() const;
    void setVoicePerturbation(float voicePerturbation);
    float velocitySensitivity() const;
    void setVelocitySensitivity(float sensitivity);
    float intonation() const;
    void setIntonation(float intonation);
    float vibratoRate() const;
    void setVibratoRate(float vibratoRate);
    float vibratoDepth() const;
    void setVibratoDepth(float vibratoDepth);
    float lpfCutoff() const;
    void setLpfCutoff(float lpfCutoff);
    float hpfCutoff() const;
    void setHpfCutoff(float hpfCutoff);
    int triggerMode() const;
    void setTriggerMode(int triggerMode);
    int syncMode() const;
    void setSyncMode(int syncMode);
    int syncLength() const;
    void setSyncLength(int syncLength);
    int syncDivision() const;
    void setSyncDivision(int syncDivision);
    int pitchBendRange() const;
    void setPitchBendRange(int range);

    size_t syllableCursor() const;
    size_t lineCursor() const;
    size_t syllableCount() const;
    size_t lineCount() const;

protected:
    void syncParameters() override;

private:
    void handleNoteOn(uint8_t note, uint8_t velocity);
    void handleNoteOff(uint8_t note);
    void compilePhrase();
    SpeechSequencer::TriggerMode triggerModeEnum() const;
    double currentFrequency() const;
    double nextFlutter(double sampleRate);

    static constexpr double OutputFadeTime { 0.006 };
    static constexpr double FadeFloor { 0.0001 };
    static constexpr double PitchGlideTime { 0.015 };
    static constexpr double ProsodyGainGlideTime { 0.015 };
    static constexpr double VibratoOnsetSeconds { 0.22 };
    static constexpr double VibratoReleaseSeconds { 0.030 };
    static constexpr double FlutterRates[] { 12.7, 7.1, 4.7 };
    static constexpr double FlutterDepth { 0.0085 };

    std::string m_name;
    std::string m_phrase;

    SpeechSequencerV2 m_sequencer;
    std::vector<ProsodyEvent> m_prosody {};
    FormantVoiceV2 m_voice;

    Lfo m_vibrato;
    CascadedSvf m_lpfL;
    CascadedSvf m_lpfR;
    CascadedSvf m_hpfL;
    CascadedSvf m_hpfR;
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;
    TrueStereoPanner m_panner;

    uint8_t m_note { 0 };
    std::set<uint8_t> m_heldNotes;
    double m_velocity { 1.0 };
    uint16_t m_pitchBend { 8192 };
    int m_pitchBendRange { 2 };

    double m_pitch { -1.0 };
    double m_pitchCoefficient { 1.0 };

    std::optional<double> m_notePitch;
    double m_portamentoCoefficient { 1.0 };
    std::array<double, std::size(FlutterRates)> m_flutterPhases {};
    double m_vibratoAge { 0.0 };
    double m_vibratoEnvelope { 0.0 };

    double m_fade { 0.0 };
    double m_fadeCoefficient { 0.0 };
    double m_prosodyGain { 1.0 };
    double m_prosodyGainCoefficient { 1.0 };
    double m_voiceIntonationScale { 1.0 };
    bool m_voiceFlutterEnabled { true };

    float m_rate { 0.25f };
    float m_portamento { 0.0f };
    float m_glide { 0.35f };
    float m_formantShift { 0.5f };
    float m_breathiness { 0.1f };
    float m_consonantLevel { 0.5f };
    float m_sibilance { 0.31f };
    float m_voiceType { 0.0f };
    float m_openQuotient { 0.5f };
    float m_voicePerturbation { 0.5f };
    float m_velocitySensitivity { 0.5f };
    float m_intonation { 0.4f };
    float m_vibratoRate { 0.3f };
    float m_vibratoDepth { 0.0f };
    float m_lpfCutoff { 1.0f };
    float m_hpfCutoff { 0.0f };
    float m_triggerMode { 0.0f };
    float m_syncMode { 0.0f };
    float m_syncLength { 16.0f };
    float m_syncDivision { 2.0f };
};

} // namespace noteahead

#endif // SPEECH_V2_DEVICE_HPP
