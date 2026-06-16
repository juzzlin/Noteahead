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

#ifndef DRUM_SYNTH_DEVICE_HPP
#define DRUM_SYNTH_DEVICE_HPP

#include "domain/devices/device.hpp"
#include "domain/devices/drum_synth_constants.hpp"
#include "domain/dsp/drum/crash_engine.hpp"
#include "domain/dsp/drum/hihat_engine.hpp"
#include "domain/dsp/drum/kick_engine.hpp"
#include "domain/dsp/drum/ride_engine.hpp"
#include "domain/dsp/drum/snare_engine.hpp"
#include "domain/dsp/drum/tom_engine.hpp"
#include "domain/dsp/oversampler.hpp"
#include "domain/effects/high_pass_filter_effect.hpp"
#include "domain/effects/low_pass_filter_effect.hpp"
#include "domain/effects/panning_effect.hpp"
#include "domain/effects/volume_effect.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace noteahead {

class DrumSynthDevice : public Device
{
public:
    explicit DrumSynthDevice(std::string name);
    virtual ~DrumSynthDevice() override = default;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    std::vector<MidiCcController> availableMidiCcControllers() const override;

    static std::string typeIdString();

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    bool hasActiveAudio() const override;

    void reset() override;
    void resetAudio() override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

    int selectedVoice() const;
    void setSelectedVoice(int index);

    uint8_t voiceNote(int index) const;

    bool updateVoiceParameter(int voiceIndex, const std::string & paramName, float value);

protected:
    void syncParameters() override;

private:
    struct Voice
    {
        std::unique_ptr<DrumEngine> engine;
        std::shared_ptr<LowPassFilterEffect> lpf;
        std::shared_ptr<HighPassFilterEffect> hpf;
        std::shared_ptr<VolumeEffect> volumeEffect;
        std::shared_ptr<PanningEffect> panningEffect;

        uint8_t midiNote { 0 };
        float level { 1.0f };
        float pan { 0.5f };
        float lpfCutoff { 1.0f };
        float hpfCutoff { 0.0f };

        void updateEffects();
    };

    std::string m_name;
    std::array<Voice, DrumSynth::NumVoices> m_voices;
    int m_selectedVoice { 0 };

    Oversampler2x m_oversamplerL;
    Oversampler2x m_oversamplerR;

    void initializeVoices();
    void addVoiceParameters(int index);
    void addKickParameters(const std::string & prefix);
    void addSnareParameters(const std::string & prefix);
    void addTomParameters(const std::string & prefix);
    void addHiHatParameters(const std::string & prefix);
    void addCymbalParameters(const std::string & prefix);

    void syncVoiceParameters(int index);
    void syncCommonEngineParameters(int index, const std::string & prefix);
    void syncKickParameters(const std::string & prefix);
    void syncSnareParameters(const std::string & prefix);
    void syncClapParameters(const std::string & prefix);
    void syncTomParameters(int index, const std::string & prefix);
    void syncHiHatParameters(int index, const std::string & prefix);
    void syncCymbalParameters(int index, const std::string & prefix);
};

} // namespace noteahead

#endif // DRUM_SYNTH_DEVICE_HPP
