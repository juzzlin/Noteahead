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

#ifndef SAMPLER_CONTROLLER_HPP
#define SAMPLER_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>
#include <QTemporaryDir>
#include <optional>

#include "../../domain/devices/sampler_device.hpp"

#include "../../application/models/sampler/sampler_pad_model.hpp"

namespace noteahead {

class AudioService;
class SamplerDevice;

class SamplerController : public DeviceController
{
    Q_OBJECT
    Q_PROPERTY(noteahead::SamplerPadModel * padModel READ padModel CONSTANT)
    Q_PROPERTY(int selectedPad READ selectedPad WRITE setSelectedPad NOTIFY selectedPadChanged)
    Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackPositionChanged)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY isFinishedChanged)
    Q_PROPERTY(double selectedPadPan READ selectedPadPan WRITE setSelectedPadPan NOTIFY selectedPadPanChanged)
    Q_PROPERTY(double selectedPadVolume READ selectedPadVolume WRITE setSelectedPadVolume NOTIFY selectedPadVolumeChanged)
    Q_PROPERTY(double selectedPadCutoff READ selectedPadCutoff WRITE setSelectedPadCutoff NOTIFY selectedPadCutoffChanged)
    Q_PROPERTY(double selectedPadHpfCutoff READ selectedPadHpfCutoff WRITE setSelectedPadHpfCutoff NOTIFY selectedPadHpfCutoffChanged)
    Q_PROPERTY(int selectedPadStartOffsetSeconds READ selectedPadStartOffsetSeconds WRITE setSelectedPadStartOffsetSeconds NOTIFY selectedPadStartOffsetChanged)
    Q_PROPERTY(int selectedPadStartOffsetMilliseconds READ selectedPadStartOffsetMilliseconds WRITE setSelectedPadStartOffsetMilliseconds NOTIFY selectedPadStartOffsetChanged)
    Q_PROPERTY(int selectedPadEndOffsetSeconds READ selectedPadEndOffsetSeconds WRITE setSelectedPadEndOffsetSeconds NOTIFY selectedPadEndOffsetChanged)
    Q_PROPERTY(int selectedPadEndOffsetMilliseconds READ selectedPadEndOffsetMilliseconds WRITE setSelectedPadEndOffsetMilliseconds NOTIFY selectedPadEndOffsetChanged)
    Q_PROPERTY(int selectedPadLoopStartSeconds READ selectedPadLoopStartSeconds WRITE setSelectedPadLoopStartSeconds NOTIFY selectedPadLoopStartChanged)
    Q_PROPERTY(int selectedPadLoopStartMilliseconds READ selectedPadLoopStartMilliseconds WRITE setSelectedPadLoopStartMilliseconds NOTIFY selectedPadLoopStartChanged)
    Q_PROPERTY(double selectedPadTune READ selectedPadTune WRITE setSelectedPadTune NOTIFY selectedPadTuneChanged)
    Q_PROPERTY(double selectedPadDetune READ selectedPadDetune WRITE setSelectedPadDetune NOTIFY selectedPadDetuneChanged)
    Q_PROPERTY(double selectedPadAttack READ selectedPadAttack WRITE setSelectedPadAttack NOTIFY selectedPadAttackChanged)
    Q_PROPERTY(double selectedPadDecay READ selectedPadDecay WRITE setSelectedPadDecay NOTIFY selectedPadDecayChanged)
    Q_PROPERTY(double selectedPadSustain READ selectedPadSustain WRITE setSelectedPadSustain NOTIFY selectedPadSustainChanged)
    Q_PROPERTY(double selectedPadRelease READ selectedPadRelease WRITE setSelectedPadRelease NOTIFY selectedPadReleaseChanged)
    Q_PROPERTY(double selectedPadCurve READ selectedPadCurve WRITE setSelectedPadCurve NOTIFY selectedPadCurveChanged)
    Q_PROPERTY(double selectedPadAttackSeconds READ selectedPadAttackSeconds NOTIFY selectedPadAttackChanged)
    Q_PROPERTY(double selectedPadHoldSeconds READ selectedPadHoldSeconds NOTIFY selectedPadHoldChanged)
    Q_PROPERTY(double selectedPadHold READ selectedPadHold WRITE setSelectedPadHold NOTIFY selectedPadHoldChanged)
    Q_PROPERTY(double selectedPadDecaySeconds READ selectedPadDecaySeconds NOTIFY selectedPadDecayChanged)
    Q_PROPERTY(double selectedPadReleaseSeconds READ selectedPadReleaseSeconds NOTIFY selectedPadReleaseChanged)
    Q_PROPERTY(bool selectedPadReverse READ selectedPadReverse WRITE setSelectedPadReverse NOTIFY selectedPadReverseChanged)
    Q_PROPERTY(bool selectedPadNormalize READ selectedPadNormalize WRITE setSelectedPadNormalize NOTIFY selectedPadNormalizeChanged)
    Q_PROPERTY(bool selectedPadLoop READ selectedPadLoop WRITE setSelectedPadLoop NOTIFY selectedPadLoopChanged)
    Q_PROPERTY(int selectedPadChokeGroup READ selectedPadChokeGroup WRITE setSelectedPadChokeGroup NOTIFY selectedPadChokeGroupChanged)
    Q_PROPERTY(double selectedPadDuration READ selectedPadDuration NOTIFY selectedPadDurationChanged)
    //! What the pad is actually heard for, trims, tuning and envelope included.
    Q_PROPERTY(double selectedPadAudibleLength READ selectedPadAudibleLength NOTIFY selectedPadDurationChanged)
    Q_PROPERTY(bool channelMode READ channelMode WRITE setChannelMode NOTIFY channelModeChanged)
    Q_PROPERTY(bool chromaticMode READ chromaticMode WRITE setChromaticMode NOTIFY chromaticModeChanged)
    Q_PROPERTY(int lpfSlope READ lpfSlope WRITE setLpfSlope NOTIFY lpfSlopeChanged)
    Q_PROPERTY(int hpfSlope READ hpfSlope WRITE setHpfSlope NOTIFY hpfSlopeChanged)
    Q_PROPERTY(bool embedWaveData READ embedWaveData WRITE setEmbedWaveData NOTIFY embedWaveDataChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QVariantList inputDevices READ inputDevices NOTIFY inputDevicesChanged)

public:
    explicit SamplerController(SamplerDevice::SamplerDeviceS sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    SamplerPadModel * padModel() const;

    using AudioServiceS = std::shared_ptr<AudioService>;
    //! Sampling is recording, so the sampler needs the service that owns the input.
    void setAudioService(AudioServiceS audioService);

    //! The input devices that can be sampled from, as {id, name} maps for the combo box.
    //!
    //! Cached rather than asked for on the spot. The dialog is built while the QML engine loads,
    //! which is before the audio service is handed over, so anything that asked at that moment got
    //! an empty list and kept it -- which is what left the Record button permanently disabled.
    QVariantList inputDevices() const;
    //! Re-scans the inputs. Called when the audio service arrives and whenever the dialog opens, so
    //! that something plugged in since last time is there.
    Q_INVOKABLE void refreshInputDevices();
    Q_INVOKABLE void setInputDevice(int deviceId);

    //! Records into the selected pad until stopRecording() is called.
    //!
    //! Goes beside the project when there is one, and into a directory of this session's own when
    //! there is not -- the pad is then marked so that saving the project writes it out rather than
    //! losing it. Refusing to record until the project has been saved, which is what the song
    //! recorder does, is the wrong trade for sampling: the sound you want is happening now.
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    bool recording() const;
    SamplerDevice::SamplerDeviceS sampler() const;
    void setSampler(SamplerDevice::SamplerDeviceS sampler);

    int selectedPad() const;
    void setSelectedPad(int selectedPad);

    double playbackPosition() const;
    bool isFinished() const;

    double selectedPadPan() const;
    void setSelectedPadPan(double pan);

    double selectedPadVolume() const;
    void setSelectedPadVolume(double volume);

    double selectedPadCutoff() const;
    void setSelectedPadCutoff(double cutoff);

    double selectedPadHpfCutoff() const;
    void setSelectedPadHpfCutoff(double cutoff);

    int selectedPadStartOffsetSeconds() const;
    void setSelectedPadStartOffsetSeconds(int seconds);

    int selectedPadStartOffsetMilliseconds() const;
    void setSelectedPadStartOffsetMilliseconds(int milliseconds);

    int selectedPadEndOffsetSeconds() const;
    void setSelectedPadEndOffsetSeconds(int seconds);

    int selectedPadEndOffsetMilliseconds() const;
    void setSelectedPadEndOffsetMilliseconds(int milliseconds);

    int selectedPadLoopStartSeconds() const;
    void setSelectedPadLoopStartSeconds(int seconds);

    int selectedPadLoopStartMilliseconds() const;
    void setSelectedPadLoopStartMilliseconds(int milliseconds);

    double selectedPadTune() const;
    void setSelectedPadTune(double tune);

    double selectedPadDetune() const;
    void setSelectedPadDetune(double detune);

    double selectedPadAttack() const;
    void setSelectedPadAttack(double attack);
    double selectedPadHold() const;
    void setSelectedPadHold(double hold);

    double selectedPadDecay() const;
    void setSelectedPadDecay(double decay);

    double selectedPadSustain() const;
    void setSelectedPadSustain(double sustain);

    double selectedPadRelease() const;
    void setSelectedPadRelease(double release);

    double selectedPadCurve() const;
    void setSelectedPadCurve(double curve);

    //! The amp envelope's segment times in seconds, as the voices run them. The waveform view draws
    //! the envelope on the same time axis as the sample, so it needs the times, not the positions.
    double selectedPadAttackSeconds() const;
    double selectedPadHoldSeconds() const;
    double selectedPadDecaySeconds() const;
    double selectedPadReleaseSeconds() const;

    bool selectedPadReverse() const;
    void setSelectedPadReverse(bool reverse);
    bool selectedPadNormalize() const;
    void setSelectedPadNormalize(bool normalize);

    bool selectedPadLoop() const;
    void setSelectedPadLoop(bool loop);

    int selectedPadChokeGroup() const;
    void setSelectedPadChokeGroup(int group);

    double selectedPadDuration() const;
    double selectedPadAudibleLength() const;

    bool channelMode() const;
    void setChannelMode(bool enabled);

    bool chromaticMode() const;
    int lpfSlope() const;
    void setLpfSlope(int value);
    int hpfSlope() const;
    void setHpfSlope(int value);
    void setChromaticMode(bool enabled);

    bool embedWaveData() const;
    void setEmbedWaveData(bool enabled);

    Q_INVOKABLE QVariantList getWaveformData(int numPoints);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestSettings() override;

    Q_INVOKABLE void loadSample(int padIndex, const QString & filePath);
    //! Duplicates sourcePad onto targetPad: sample, pad settings and per-pad insert effects.
    Q_INVOKABLE void copyPad(int sourcePad, int targetPad);
    //! One entry per loaded pad, with padIndex, note, noteName and fileName. Feeds CopyPadDialog.
    Q_INVOKABLE QVariantList loadedPads() const;
    Q_INVOKABLE void clearSample(int padIndex);
    Q_INVOKABLE void playSample(int padIndex, double velocity = 1.0);
    Q_INVOKABLE void stopSample(int padIndex);
    Q_INVOKABLE void updatePlaybackStatus();

signals:
    void selectedPadChanged();
    void playbackPositionChanged();
    void isFinishedChanged();
    void selectedPadPanChanged();
    void selectedPadVolumeChanged();
    void selectedPadCutoffChanged();
    void selectedPadHpfCutoffChanged();
    void selectedPadStartOffsetChanged();
    void selectedPadEndOffsetChanged();
    void selectedPadLoopStartChanged();
    void selectedPadTuneChanged();
    void selectedPadDetuneChanged();
    void selectedPadAttackChanged();
    void selectedPadHoldChanged();
    void selectedPadDecayChanged();
    void selectedPadSustainChanged();
    void selectedPadReleaseChanged();
    void selectedPadCurveChanged();
    void selectedPadReverseChanged();
    void selectedPadNormalizeChanged();
    void selectedPadLoopChanged();
    void selectedPadChokeGroupChanged();
    void selectedPadDurationChanged();
    void channelModeChanged();
    void chromaticModeChanged();
    void lpfSlopeChanged();
    void hpfSlopeChanged();
    void embedWaveDataChanged();
    void recordingChanged();
    void inputDevicesChanged();
    void samplerChanged();

private:
    //! One offset as the whole-second and millisecond halves the dialog's two spin boxes edit.
    struct OffsetParts
    {
        int seconds = 0;
        int milliseconds = 0;
    };

    static OffsetParts splitSeconds(double seconds);

    int noteForPad(int padIndex) const;

    //! The note of the selected pad, or nothing when no pad is selected.
    std::optional<uint8_t> selectedNote() const;

    //! Puts what was just recorded onto the pad it was recorded for.
    void onRecordingFinished(const QString & filePath);

    SamplerDevice::SamplerDeviceS m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
    int m_selectedPad = 0;

    AudioServiceS m_audioService;
    QVariantList m_inputDevices;
    //! Where recordings go when the project has nowhere of its own yet. Lives as long as this
    //! session, which is exactly as long as the recordings in it are worth anything.
    std::unique_ptr<QTemporaryDir> m_recordingDirectory;
    //! The pad the running recording was started for, so that selecting another one mid-take does
    //! not land the sample somewhere the user was not looking when they pressed record.
    std::optional<int> m_recordingPad;
    //! Whether the running recording will need writing out when the project is saved.
    bool m_recordingIsEphemeral = false;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
