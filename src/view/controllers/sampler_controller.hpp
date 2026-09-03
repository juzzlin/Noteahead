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
#include <optional>

#include "../../domain/devices/sampler_device.hpp"

#include "../../application/models/sampler/sampler_pad_model.hpp"

namespace noteahead {

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
    Q_PROPERTY(bool selectedPadEndOffsetEnabled READ selectedPadEndOffsetEnabled WRITE setSelectedPadEndOffsetEnabled NOTIFY selectedPadEndOffsetChanged)
    Q_PROPERTY(int selectedPadEndOffsetSeconds READ selectedPadEndOffsetSeconds WRITE setSelectedPadEndOffsetSeconds NOTIFY selectedPadEndOffsetChanged)
    Q_PROPERTY(int selectedPadEndOffsetMilliseconds READ selectedPadEndOffsetMilliseconds WRITE setSelectedPadEndOffsetMilliseconds NOTIFY selectedPadEndOffsetChanged)
    Q_PROPERTY(double selectedPadTune READ selectedPadTune WRITE setSelectedPadTune NOTIFY selectedPadTuneChanged)
    Q_PROPERTY(double selectedPadDetune READ selectedPadDetune WRITE setSelectedPadDetune NOTIFY selectedPadDetuneChanged)
    Q_PROPERTY(double selectedPadAttack READ selectedPadAttack WRITE setSelectedPadAttack NOTIFY selectedPadAttackChanged)
    Q_PROPERTY(double selectedPadDecay READ selectedPadDecay WRITE setSelectedPadDecay NOTIFY selectedPadDecayChanged)
    Q_PROPERTY(double selectedPadSustain READ selectedPadSustain WRITE setSelectedPadSustain NOTIFY selectedPadSustainChanged)
    Q_PROPERTY(double selectedPadRelease READ selectedPadRelease WRITE setSelectedPadRelease NOTIFY selectedPadReleaseChanged)
    Q_PROPERTY(bool selectedPadReverse READ selectedPadReverse WRITE setSelectedPadReverse NOTIFY selectedPadReverseChanged)
    Q_PROPERTY(bool selectedPadLoop READ selectedPadLoop WRITE setSelectedPadLoop NOTIFY selectedPadLoopChanged)
    Q_PROPERTY(int selectedPadChokeGroup READ selectedPadChokeGroup WRITE setSelectedPadChokeGroup NOTIFY selectedPadChokeGroupChanged)
    Q_PROPERTY(double selectedPadDuration READ selectedPadDuration NOTIFY selectedPadDurationChanged)
    Q_PROPERTY(bool channelMode READ channelMode WRITE setChannelMode NOTIFY channelModeChanged)
    Q_PROPERTY(bool chromaticMode READ chromaticMode WRITE setChromaticMode NOTIFY chromaticModeChanged)
    Q_PROPERTY(bool embedWaveData READ embedWaveData WRITE setEmbedWaveData NOTIFY embedWaveDataChanged)

public:
    explicit SamplerController(SamplerDevice::SamplerDeviceS sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    SamplerPadModel * padModel() const;
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

    bool selectedPadEndOffsetEnabled() const;
    void setSelectedPadEndOffsetEnabled(bool enabled);

    int selectedPadEndOffsetSeconds() const;
    void setSelectedPadEndOffsetSeconds(int seconds);

    int selectedPadEndOffsetMilliseconds() const;
    void setSelectedPadEndOffsetMilliseconds(int milliseconds);

    double selectedPadTune() const;
    void setSelectedPadTune(double tune);

    double selectedPadDetune() const;
    void setSelectedPadDetune(double detune);

    double selectedPadAttack() const;
    void setSelectedPadAttack(double attack);

    double selectedPadDecay() const;
    void setSelectedPadDecay(double decay);

    double selectedPadSustain() const;
    void setSelectedPadSustain(double sustain);

    double selectedPadRelease() const;
    void setSelectedPadRelease(double release);

    bool selectedPadReverse() const;
    void setSelectedPadReverse(bool reverse);

    bool selectedPadLoop() const;
    void setSelectedPadLoop(bool loop);

    int selectedPadChokeGroup() const;
    void setSelectedPadChokeGroup(int group);

    double selectedPadDuration() const;

    bool channelMode() const;
    void setChannelMode(bool enabled);

    bool chromaticMode() const;
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
    void selectedPadTuneChanged();
    void selectedPadDetuneChanged();
    void selectedPadAttackChanged();
    void selectedPadDecayChanged();
    void selectedPadSustainChanged();
    void selectedPadReleaseChanged();
    void selectedPadReverseChanged();
    void selectedPadLoopChanged();
    void selectedPadChokeGroupChanged();
    void selectedPadDurationChanged();
    void channelModeChanged();
    void chromaticModeChanged();
    void embedWaveDataChanged();
    void samplerChanged();

private:
    int noteForPad(int padIndex) const;

    //! The note of the selected pad, or nothing when no pad is selected.
    std::optional<uint8_t> selectedNote() const;

    SamplerDevice::SamplerDeviceS m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
    int m_selectedPad = 0;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
