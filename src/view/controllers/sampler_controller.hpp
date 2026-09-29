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
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>
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
    Q_PROPERTY(bool selectedPadMono READ selectedPadMono WRITE setSelectedPadMono NOTIFY selectedPadMonoChanged)
    Q_PROPERTY(bool metronomeEnabled READ metronomeEnabled WRITE setMetronomeEnabled NOTIFY metronomeEnabledChanged)
    Q_PROPERTY(bool clickDuringTake READ clickDuringTake WRITE setClickDuringTake NOTIFY clickDuringTakeChanged)
    Q_PROPERTY(int preCountBars READ preCountBars WRITE setPreCountBars NOTIFY preCountBarsChanged)
    Q_PROPERTY(int metronomeBeatsPerBar READ metronomeBeatsPerBar WRITE setMetronomeBeatsPerBar NOTIFY metronomeBeatsPerBarChanged)
    //! Beats of the pre-count still to come, or zero when one is not running. Polled by the clock in
    //! the wave view, which counts them down.
    Q_PROPERTY(int countInBeatsRemaining READ countInBeatsRemaining NOTIFY countInBeatsRemainingChanged)
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

public:
    explicit SamplerController(SamplerDevice::SamplerDeviceS sampler, QObject * parent = nullptr);
    ~SamplerController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    SamplerPadModel * padModel() const;

    using AudioServiceS = std::shared_ptr<AudioService>;
    //! Sampling is recording, so the sampler needs the service that owns the input.
    void setAudioService(AudioServiceS audioService);

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
    bool selectedPadMono() const;

    bool metronomeEnabled() const;
    void setMetronomeEnabled(bool enabled);
    bool clickDuringTake() const;
    void setClickDuringTake(bool enabled);
    int preCountBars() const;
    void setPreCountBars(int bars);
    int metronomeBeatsPerBar() const;
    void setMetronomeBeatsPerBar(int beats);
    int countInBeatsRemaining() const;
    void setSelectedPadLoop(bool loop);
    void setSelectedPadMono(bool mono);

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
    //! Moves the pad's trims in past the silence at either end. Reversible: it sets the markers.
    Q_INVOKABLE void autoTrimPad(int padIndex);
    //! Writes the pad's trimmed range out as a new file and plays that instead. Not reversible.
    Q_INVOKABLE void cropPadToTrim(int padIndex);
    //! The note a pad sits on, which in chromatic mode is the note its audio sounds and the bottom of
    //! the range it covers.
    Q_INVOKABLE int padNote(int padIndex) const;
    //! Moves a chromatic pad, taking its audio with it. False when the note is out of range or
    //! another pad is already there.
    Q_INVOKABLE bool setPadNote(int padIndex, int midiNote);
    //! "C-4" and the like, for naming a note in the UI without duplicating NoteConverter in QML.
    Q_INVOKABLE QString noteName(int midiNote) const;
    //! How long the take running now has lasted, in seconds. Zero when nothing is recording.
    //!
    //! Wall clock from the moment recording started, which is what "how long have I been playing"
    //! asks. Polled rather than signalled: it changes continuously, so a notify per value would be a
    //! signal per frame for something a readout only shows to a tenth of a second.
    Q_INVOKABLE double recordingSeconds() const;

    //! What the meters beside the wave view show: leftPeakDb, leftRmsDb, rightPeakDb, rightRmsDb.
    //!
    //! The record input while recording, and the Sampler's own output -- after its inserts and its
    //! fader, the same tap the mixer reads -- the rest of the time. One call, so both bars are always
    //! from the same moment.
    Q_INVOKABLE QVariantMap meterLevels() const;
    //! Turns both taps on, for as long as the dialog showing them is open.
    Q_INVOKABLE void setMetersActive(bool active);

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
    void selectedPadMonoChanged();
    void metronomeEnabledChanged();
    void clickDuringTakeChanged();
    void preCountBarsChanged();
    void metronomeBeatsPerBarChanged();
    void countInBeatsRemainingChanged();
    void selectedPadChokeGroupChanged();
    void selectedPadDurationChanged();
    void channelModeChanged();
    void chromaticModeChanged();
    void lpfSlopeChanged();
    void hpfSlopeChanged();
    void embedWaveDataChanged();
    void recordingChanged();
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
    //! The recorder is running: start the pre-count, if one was asked for.
    void onRecordingStarted();
    //! Watches the metronome for the end of the pre-count, which is decided on the audio thread.
    void pollCountIn();

    SamplerDevice::SamplerDeviceS m_sampler;
    std::unique_ptr<SamplerPadModel> m_padModel;
    int m_selectedPad = 0;

    AudioServiceS m_audioService;
    //! Where recordings go when the project has nowhere of its own yet. Lives as long as this
    //! session, which is exactly as long as the recordings in it are worth anything.
    std::unique_ptr<QTemporaryDir> m_recordingDirectory;
    //! The pad the running recording was started for, so that selecting another one mid-take does
    //! not land the sample somewhere the user was not looking when they pressed record.
    std::optional<int> m_recordingPad;
    QElapsedTimer m_recordingElapsed;

    //! Started when a take starts and stopped when it ends. Polls the metronome, which decides on the
    //! audio thread when the pre-count is through and cannot signal it out.
    QTimer * m_countInPoller {};
    bool m_metronomeEnabled;
    bool m_clickDuringTake;
    int m_preCountBars;
    int m_metronomeBeatsPerBar = 4;
    //! Seconds of pre-count in the take running now, which is what it gets trimmed by. Zero when the
    //! take was recorded without one.
    double m_countInSeconds = 0.0;
    int m_countInBeatsRemaining = 0;
    //! Whether the running recording will need writing out when the project is saved.
    bool m_recordingIsEphemeral = false;
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_HPP
