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

#ifndef SAMPLER_DEVICE_HPP
#define SAMPLER_DEVICE_HPP

#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../dsp/high_pass_filter.hpp"
#include "../dsp/low_pass_filter.hpp"
#include "../dsp/panning.hpp"
#include "../dsp/volume.hpp"
#include "../effects/effect.hpp"
#include "../tracker/parameter_container.hpp"
#include "device.hpp"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class SamplerDevice : public Device
{
public:
    using SamplerDeviceS = std::shared_ptr<SamplerDevice>;
    static constexpr size_t maxSamples = 128;
    static constexpr int padCount = 16;
    static constexpr uint8_t padStartNote = 36;
    using AudioFileReaderU = std::unique_ptr<AudioFileReader>;

    //! First MIDI CC of each per-pad parameter block: the CC for a pad is the block start plus the pad
    //! index, so the blocks span 16..31, 32..47, 48..63 and 102..117.
    //!
    //! 32..47 are the LSB counterparts of controllers 0..15, which this device never reads, and none of
    //! the blocks touch the device-wide CCs (7 Fader, 10 Pan, 74 LPF, 81 HPF, 121 Reset).
    static constexpr uint8_t padPanCcStart = 16;
    static constexpr uint8_t padVolumeCcStart = 32;
    static constexpr uint8_t padCutoffCcStart = 48;
    static constexpr uint8_t padHpfCutoffCcStart = 102;

    explicit SamplerDevice(std::string name, AudioFileReaderU audioFileReader = nullptr);
    ~SamplerDevice() override;

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

    struct Sample : public ParameterContainer
    {
        Sample();

        std::string filePath;
        std::shared_ptr<const std::vector<float>> data;
        int channels = 0;
        int sampleRate = 0;

        float pan = 0.5f;
        //! Fader position, not a gain: seeded to unity in the constructor, mapped by mapFader().
        float volume = 1.0f;
        float cutoff = 1.0f;
        float hpfCutoff = 0.0f;
        double startOffset = 0.0;

        // Per-pad insert effect rack. Shared so Sample stays copyable (saveState/restoreState deep-copy)
        // and so every voice/note playing this sample runs through the same stateful effect chain.
        std::shared_ptr<EffectRack> effectRack;
    };

    void loadSample(uint8_t note, const std::string & filePath);
    //! Duplicates the sample of sourceNote onto targetNote: the sample data, the pad settings and an
    //! independent clone of the per-pad insert rack. Does nothing if the source pad is empty.
    void copySample(uint8_t sourceNote, uint8_t targetNote);
    void clearSample(uint8_t note);
    const Sample * sample(uint8_t note) const;
    std::string absoluteFilePath(uint8_t note) const;

    float samplePan(uint8_t note) const;
    void setSamplePan(uint8_t note, float pan);

    float sampleVolume(uint8_t note) const;
    void setSampleVolume(uint8_t note, float volume);

    float sampleCutoff(uint8_t note) const;
    void setSampleCutoff(uint8_t note, float cutoff);

    float sampleHpfCutoff(uint8_t note) const;
    void setSampleHpfCutoff(uint8_t note, float cutoff);

    double sampleStartOffset(uint8_t note) const;
    void setSampleStartOffset(uint8_t note, double offset);

    double sampleDuration(uint8_t note) const;

    // Per-pad insert effect rack for the given note, created lazily on first access.
    EffectRack & sampleEffectRack(uint8_t note);

    bool channelMode() const;
    void setChannelMode(bool enabled);

    bool chromaticMode() const;
    void setChromaticMode(bool enabled);

    //! Maps a pad index to a MIDI note. The two modes address the same shared per-note sample array with
    //! different layouts, so samples for both modes coexist and are all serialized; the modes are not meant
    //! to be used simultaneously. The layouts overlap only at notes 36 and 48:
    //!
    //!   Mode        Pad -> MIDI note   Notes used
    //!   ---------   ----------------   -----------------------------------
    //!   Drum        36 + pad           36..51
    //!   Chromatic   12 * pad           0, 12, 24, 36, 48, 60, ... (octave C roots)
    //!
    //! The chromatic layout runs past the end of the sample array on the topmost pads, so the note is
    //! returned unclamped: callers that index the array have to check it against maxSamples.
    int noteForPad(int padIndex) const;

    // Resolves the sample that covers the given note in chromatic mode and, via rootNote, the octave root it
    // is pitched from. Returns nullptr if no sample is set. The lowest set root extends down and the highest
    // set root extends up, so a single set sample covers the whole range.
    const Sample * coveringSample(uint8_t note, uint8_t & rootNote) const;

    // Playback speed ratio for the given note in chromatic mode: 2^((note - coveringRoot) / 12).
    double chromaticPitchRatio(uint8_t note) const;

    bool embedWaveData() const;
    void setEmbedWaveData(bool enabled);

    std::map<QString, QString> getFilesToEmbed() const;

    void setPan(float pan) override;
    void setVolume(float volume) override;

    float gain() const;
    void setGain(float gain) override;

    double playbackPosition(uint8_t note) const;
    bool isFinished(uint8_t note) const;

    void setProjectPath(const std::string & projectPath);

    using PathResolver = std::function<QString(const QString &)>;
    void setPathResolver(PathResolver resolver);

    //! Snapshots the pads' samples on top of the parameters the base class remembers.
    void saveState() override;
    void restoreState() override;

private:
    struct Voice;
    void updateVoiceEffects(Voice & voice);

    //! The pad a per-pad MIDI CC addresses, plus the parameter it drives and the already-mapped value.
    struct PadCcTarget
    {
        int padIndex = 0;
        std::string parameterName;
        float value = 0.0f;
    };

    //! Resolves a controller number against the per-pad CC blocks. Nothing when it falls outside them.
    std::optional<PadCcTarget> padCcTarget(uint8_t controller, uint8_t value) const;

    //! Copies a pad's parameters into the plain fields the voices read.
    void syncSampleFields(Sample & sample);

    //! Writes an automated value into one pad's parameter and refreshes the voices playing that
    //! pad. The authored value is left alone, which is what clearAutomationInternal() restores.

    bool updatePadParameter(int note, const std::string & parameterName, float value);

    //! Copy of the given sample that shares nothing mutable with it: the pad settings and the insert
    //! rack are duplicated, only the immutable sample data is shared.
    std::unique_ptr<Sample> cloneSample(const Sample & source) const;

    //! Silences every voice playing through a sample that is about to be replaced or destroyed,
    //! or through any sample at all when given nothing.
    //!
    //! A voice holds a raw pointer to its sample, so a sample outliving nothing is not enough: it
    //! has to outlive every voice reading it. Call this under the lock, before the sample goes.
    void stopVoicesUsing(const Sample * sample);
    void syncParameters() override;
    bool clearAutomationInternal() override;

    struct Voice
    {
        Voice();

        uint8_t note = 0;
        Sample * sample = nullptr;
        double position = 0.0;
        double pitchRatio = 1.0;
        float velocity = 1.0f;
        float pan = 0.5f;
        float cutoff = 1.0f;
        float hpfCutoff = 0.0f;

        std::shared_ptr<LowPassFilter> lpf;
        std::shared_ptr<HighPassFilter> hpf;
        std::shared_ptr<Volume> volumeEffect;
        std::shared_ptr<Panning> panningEffect;
        std::vector<std::shared_ptr<Effect>> effects;

        bool active = false;
        bool releasing = false;
        float releaseGain = 1.0f;
    };

    std::array<std::unique_ptr<Sample>, maxSamples> m_samples;
    std::array<std::unique_ptr<Sample>, maxSamples> m_savedSamples;
    std::vector<Voice> m_voices;

    // Reusable audio-thread scratch buffers, kept as members so processAudio() does not allocate on
    // every callback. m_padBuffers is a pool of per-pad sub-mix buffers reused across callbacks.
    std::vector<double> m_mixBuffer;
    std::vector<std::pair<Sample *, std::vector<double>>> m_padBuffers;

    std::string m_name;
    float m_globalCutoff = 1.0f;
    float m_globalHpfCutoff = 0.0f;
    float m_authoredGlobalCutoff = 1.0f;
    float m_authoredGlobalHpfCutoff = 0.0f;
    bool m_channelMode = false;
    bool m_chromaticMode = false;
    bool m_embedWaveData = false;
    std::string m_projectPath;
    PathResolver m_pathResolver;
    AudioFileReaderU m_audioFileReader;
    const size_t m_maxVoices = 32;
};

} // namespace noteahead

#endif // SAMPLER_DEVICE_HPP
