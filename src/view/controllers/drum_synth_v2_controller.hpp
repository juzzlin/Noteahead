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

#ifndef DRUM_SYNTH_V2_CONTROLLER_HPP
#define DRUM_SYNTH_V2_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QTimer>
#include <QVariantList>

#include <memory>
#include <string>

namespace noteahead {

class DeviceService;
class DrumSynthV2Device;

class DrumSynthV2Controller : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int selectedVoice READ selectedVoice WRITE setSelectedVoice NOTIFY selectedVoiceChanged)
    //! The selected voice's picture, and the two lengths that go with it. Pushed rather than
    //! pulled: a Sampler pad's picture changes only when its file does, so QML can ask for one when
    //! it likes; a drum voice's changes with every knob, and rendering one costs tens of
    //! milliseconds, so the controller decides when to redraw and tells the view.
    Q_PROPERTY(QVariantList waveformData READ waveformData NOTIFY waveformChanged)
    //! What the picture spans, in seconds.
    Q_PROPERTY(double waveformDuration READ waveformDuration NOTIFY waveformChanged)
    //! What is actually heard, in seconds, envelope and effects included.
    Q_PROPERTY(double audibleLength READ audibleLength NOTIFY waveformChanged)
    Q_PROPERTY(int lpfSlope READ lpfSlope WRITE setLpfSlope NOTIFY lpfSlopeChanged)
    Q_PROPERTY(int hpfSlope READ hpfSlope WRITE setHpfSlope NOTIFY hpfSlopeChanged)

    // Selected Voice Parameters
    Q_PROPERTY(int voiceLevel READ voiceLevel WRITE setVoiceLevel NOTIFY voiceLevelChanged)
    Q_PROPERTY(int voicePan READ voicePan WRITE setVoicePan NOTIFY voicePanChanged)
    Q_PROPERTY(int voiceLpfCutoff READ voiceLpfCutoff WRITE setVoiceLpfCutoff NOTIFY voiceLpfCutoffChanged)
    Q_PROPERTY(int voiceHpfCutoff READ voiceHpfCutoff WRITE setVoiceHpfCutoff NOTIFY voiceHpfCutoffChanged)
    Q_PROPERTY(int voiceTune READ voiceTune WRITE setVoiceTune NOTIFY voiceTuneChanged)
    Q_PROPERTY(int voiceDecay READ voiceDecay WRITE setVoiceDecay NOTIFY voiceDecayChanged)
    Q_PROPERTY(int voiceAttack READ voiceAttack WRITE setVoiceAttack NOTIFY voiceAttackChanged)

    // Per-voice amp envelope
    Q_PROPERTY(int voiceAmpAttack READ voiceAmpAttack WRITE setVoiceAmpAttack NOTIFY voiceAmpAttackChanged)
    Q_PROPERTY(int voiceAmpHold READ voiceAmpHold WRITE setVoiceAmpHold NOTIFY voiceAmpHoldChanged)
    Q_PROPERTY(int voiceAmpDecay READ voiceAmpDecay WRITE setVoiceAmpDecay NOTIFY voiceAmpDecayChanged)
    Q_PROPERTY(int voiceAmpSustain READ voiceAmpSustain WRITE setVoiceAmpSustain NOTIFY voiceAmpSustainChanged)
    Q_PROPERTY(int voiceAmpRelease READ voiceAmpRelease WRITE setVoiceAmpRelease NOTIFY voiceAmpReleaseChanged)
    Q_PROPERTY(int voiceAmpCurve READ voiceAmpCurve WRITE setVoiceAmpCurve NOTIFY voiceAmpCurveChanged)

    // Kick Specific
    Q_PROPERTY(int kickAttack READ kickAttack WRITE setKickAttack NOTIFY kickAttackChanged)
    Q_PROPERTY(int kickClickTune READ kickClickTune WRITE setKickClickTune NOTIFY kickClickTuneChanged)
    Q_PROPERTY(int kickPitchDepth READ kickPitchDepth WRITE setKickPitchDepth NOTIFY kickPitchDepthChanged)
    Q_PROPERTY(int kickPitchDecay READ kickPitchDecay WRITE setKickPitchDecay NOTIFY kickPitchDecayChanged)

    // Snare Specific
    Q_PROPERTY(int snareSnappy READ snareSnappy WRITE setSnareSnappy NOTIFY snareSnappyChanged)
    Q_PROPERTY(int snareTone READ snareTone WRITE setSnareTone NOTIFY snareToneChanged)

    // Tom Specific
    Q_PROPERTY(int tomPitchDepth READ tomPitchDepth WRITE setTomPitchDepth NOTIFY tomPitchDepthChanged)
    Q_PROPERTY(int tomPitchDecay READ tomPitchDecay WRITE setTomPitchDecay NOTIFY tomPitchDecayChanged)

    // HiHat / Cymbal Specific
    Q_PROPERTY(int voiceResonance READ voiceResonance WRITE setVoiceResonance NOTIFY voiceResonanceChanged)

    // Global

    // UI Helpers
    Q_PROPERTY(bool isKick READ isKick NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isSnare READ isSnare NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isTom READ isTom NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isCymbal READ isCymbal NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool hasResonance READ hasResonance NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool hasAttack READ hasAttack NOTIFY selectedVoiceChanged)
    Q_PROPERTY(QVariantList activeNotes READ activeNotes NOTIFY activeNotesChanged)

public:
    explicit DrumSynthV2Controller(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    Q_INVOKABLE void setDevice(const QString & deviceName);

    int selectedVoice() const;
    int lpfSlope() const;
    void setLpfSlope(int value);
    int hpfSlope() const;
    void setHpfSlope(int value);
    void setSelectedVoice(int index);

    QVariantList waveformData() const;
    double waveformDuration() const;
    double audibleLength() const;

    //! How many points the view has room for, and whether it is on screen at all. Nothing is
    //! rendered until the view says both: a knob turned with the dialog shut costs nothing.
    Q_INVOKABLE void setWaveformRequest(int peakCount, bool visible);

    int voiceLevel() const;
    void setVoiceLevel(int value);

    int voicePan() const;
    void setVoicePan(int value);

    int voiceLpfCutoff() const;
    void setVoiceLpfCutoff(int value);

    int voiceHpfCutoff() const;
    void setVoiceHpfCutoff(int value);

    int voiceTune() const;
    void setVoiceTune(int value);

    int voiceDecay() const;
    void setVoiceDecay(int value);

    int voiceAttack() const;
    void setVoiceAttack(int value);
    int voiceAmpAttack() const;
    void setVoiceAmpAttack(int value);
    int voiceAmpHold() const;
    void setVoiceAmpHold(int value);
    int voiceAmpDecay() const;
    int voiceAmpSustain() const;
    void setVoiceAmpSustain(int value);
    int voiceAmpRelease() const;
    void setVoiceAmpRelease(int value);
    void setVoiceAmpDecay(int value);
    int voiceAmpCurve() const;
    void setVoiceAmpCurve(int value);

    int kickAttack() const;
    void setKickAttack(int value);

    int kickClickTune() const;
    void setKickClickTune(int value);

    int kickPitchDepth() const;
    void setKickPitchDepth(int value);

    int kickPitchDecay() const;
    void setKickPitchDecay(int value);

    int snareSnappy() const;
    void setSnareSnappy(int value);

    int snareTone() const;
    void setSnareTone(int value);

    int tomPitchDepth() const;
    void setTomPitchDepth(int value);

    int tomPitchDecay() const;
    void setTomPitchDecay(int value);

    int voiceResonance() const;
    void setVoiceResonance(int value);

    bool isKick() const;
    bool isSnare() const;
    bool isTom() const;
    bool isCymbal() const;
    bool hasResonance() const;
    bool hasAttack() const;
    QVariantList activeNotes() const;

    Q_INVOKABLE void requestSettings() override;
    Q_INVOKABLE void playVoice(int index);

signals:
    void selectedVoiceChanged();
    void waveformChanged();
    void lpfSlopeChanged();
    void hpfSlopeChanged();
    void voiceLevelChanged();
    void voicePanChanged();
    void voiceLpfCutoffChanged();
    void voiceHpfCutoffChanged();
    void voiceTuneChanged();
    void voiceDecayChanged();
    void voiceAttackChanged();
    void voiceAmpAttackChanged();
    void voiceAmpHoldChanged();
    void voiceAmpDecayChanged();
    void voiceAmpSustainChanged();
    void voiceAmpReleaseChanged();
    void voiceAmpCurveChanged();
    void kickAttackChanged();
    void kickClickTuneChanged();
    void kickPitchDepthChanged();
    void kickPitchDecayChanged();
    void snareSnappyChanged();
    void snareToneChanged();
    void tomPitchDepthChanged();
    void tomPitchDecayChanged();
    void voiceResonanceChanged();
    void activeNotesChanged();

private:
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<DrumSynthV2Device> m_device;

    //! Coalesces the redraws. A knob drag emits a change per pixel of travel, and each one would
    //! otherwise pay for a render; restarting this on every change collapses a drag into one.
    QTimer m_waveformTimer;
    //! Redraws now rather than after the wait. Picking a voice is one deliberate act rather than a
    //! stream of them, and a quarter of a second of blank would read as the dialog being slow.
    void renderWaveform();
    void scheduleWaveform();

    QVariantList m_waveformData;
    double m_waveformDuration { 0.0 };
    double m_audibleLength { 0.0 };
    int m_waveformPeakCount { 0 };
    bool m_waveformVisible { false };
    int m_selectedVoice { 0 };

    std::string currentVoicePrefix() const;
};

} // namespace noteahead

#endif // DRUM_SYNTH_V2_CONTROLLER_HPP
