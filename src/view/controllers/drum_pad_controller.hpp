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

#ifndef DRUM_PAD_CONTROLLER_HPP
#define DRUM_PAD_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QTimer>
#include <QVariantList>

#include <memory>
#include <string>

namespace noteahead {

class DeviceService;
class DrumPadDevice;

//! What every drum machine's dialog has, whatever plays on its pads: which pad is selected, its
//! level, pan and filters, its amp envelope, the waveform of it and the playhead running over it.
//! A subclass adds the controls of its own engines and says which device it drives.
class DrumPadController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int selectedVoice READ selectedVoice WRITE setSelectedVoice NOTIFY selectedVoiceChanged)
    //! The selected voice's picture, and the two lengths that go with it. Pushed rather than
    //! pulled: a Sampler pad's picture changes only when its file does, so QML can ask for one when
    //! it likes; a drum voice's changes with every knob, and rendering one costs tens of
    //! milliseconds, so the controller decides when to redraw and tells the view.
    //! Where the playhead sits, 0..1 of the picture, and whether there is anything to draw it for.
    Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackStatusChanged)
    Q_PROPERTY(bool voiceSounding READ voiceSounding NOTIFY playbackStatusChanged)
    Q_PROPERTY(QVariantList waveformData READ waveformData NOTIFY waveformChanged)
    //! What the picture spans, in seconds.
    Q_PROPERTY(double waveformDuration READ waveformDuration NOTIFY waveformChanged)
    //! What is actually heard, in seconds, envelope and effects included.
    Q_PROPERTY(double audibleLength READ audibleLength NOTIFY waveformChanged)
    //! The amp envelope the picture was rendered with, for the overlay drawn on it: segment times
    //! in seconds, on the same axis as the picture, and sustain and curve on 0..1. Taken when the
    //! picture is, not followed live, so that a knob drag moves neither until it has settled and
    //! the overlay never runs ahead of the waveform underneath it.
    Q_PROPERTY(double waveformEnvelopeAttack READ waveformEnvelopeAttack NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeHold READ waveformEnvelopeHold NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeDecay READ waveformEnvelopeDecay NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeSustain READ waveformEnvelopeSustain NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeRelease READ waveformEnvelopeRelease NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeCurve READ waveformEnvelopeCurve NOTIFY waveformChanged)
    Q_PROPERTY(int lpfSlope READ lpfSlope WRITE setLpfSlope NOTIFY lpfSlopeChanged)
    Q_PROPERTY(int hpfSlope READ hpfSlope WRITE setHpfSlope NOTIFY hpfSlopeChanged)

    // Selected Voice Parameters
    // Selected Voice Parameters
    Q_PROPERTY(int voiceLevel READ voiceLevel WRITE setVoiceLevel NOTIFY voiceLevelChanged)
    Q_PROPERTY(int voicePan READ voicePan WRITE setVoicePan NOTIFY voicePanChanged)
    Q_PROPERTY(int voiceLpfCutoff READ voiceLpfCutoff WRITE setVoiceLpfCutoff NOTIFY voiceLpfCutoffChanged)
    Q_PROPERTY(int voiceHpfCutoff READ voiceHpfCutoff WRITE setVoiceHpfCutoff NOTIFY voiceHpfCutoffChanged)

    // Per-voice amp envelope
    Q_PROPERTY(int voiceAmpAttack READ voiceAmpAttack WRITE setVoiceAmpAttack NOTIFY voiceAmpAttackChanged)
    Q_PROPERTY(int voiceAmpHold READ voiceAmpHold WRITE setVoiceAmpHold NOTIFY voiceAmpHoldChanged)
    Q_PROPERTY(int voiceAmpDecay READ voiceAmpDecay WRITE setVoiceAmpDecay NOTIFY voiceAmpDecayChanged)
    Q_PROPERTY(int voiceAmpSustain READ voiceAmpSustain WRITE setVoiceAmpSustain NOTIFY voiceAmpSustainChanged)
    Q_PROPERTY(int voiceAmpRelease READ voiceAmpRelease WRITE setVoiceAmpRelease NOTIFY voiceAmpReleaseChanged)
    Q_PROPERTY(int voiceAmpCurve READ voiceAmpCurve WRITE setVoiceAmpCurve NOTIFY voiceAmpCurveChanged)

    Q_PROPERTY(QVariantList activeNotes READ activeNotes NOTIFY activeNotesChanged)

public:
    explicit DrumPadController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    Q_INVOKABLE void setDevice(const QString & deviceName);

    int selectedVoice() const;
    int lpfSlope() const;
    void setLpfSlope(int value);
    int hpfSlope() const;
    void setHpfSlope(int value);
    void setSelectedVoice(int index);

    double waveformEnvelopeAttack() const;
    double waveformEnvelopeHold() const;
    double waveformEnvelopeDecay() const;
    double waveformEnvelopeSustain() const;
    double waveformEnvelopeRelease() const;
    double waveformEnvelopeCurve() const;

    double playbackPosition() const;
    bool voiceSounding() const;
    //! Polled by the view while it is showing, as the Sampler's own waveform polls its pad: the
    //! playhead moves with the audio, which nothing else in the device has reason to signal about.
    Q_INVOKABLE void updatePlaybackStatus();

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

    QVariantList activeNotes() const;

    Q_INVOKABLE void requestSettings() override;
    Q_INVOKABLE void playVoice(int index);
    //! How many voices the device has. The pad grid is built from this rather than from the length
    //! of its own label list, so a voice added in the domain cannot quietly fail to get a pad.
    Q_INVOKABLE int voiceCount() const;
    //! The device's own name for a voice, as a fallback for a pad with no short label of its own.
    Q_INVOKABLE QString voiceName(int index) const;

signals:
    void selectedVoiceChanged();
    void waveformChanged();
    void playbackStatusChanged();
    void lpfSlopeChanged();
    void hpfSlopeChanged();
    void voiceLevelChanged();
    void voicePanChanged();
    void voiceLpfCutoffChanged();
    void voiceHpfCutoffChanged();
    void voiceAmpAttackChanged();
    void voiceAmpHoldChanged();
    void voiceAmpDecayChanged();
    void voiceAmpSustainChanged();
    void voiceAmpReleaseChanged();
    void voiceAmpCurveChanged();
    void activeNotesChanged();

protected:
    //! Whether @p device is the kind this controller drives. Called with a DrumPadDevice already.
    virtual bool acceptsDevice(const DeviceS & device) const = 0;

    std::string currentVoicePrefix() const;

    std::shared_ptr<DrumPadDevice> m_device;
    int m_selectedVoice { 0 };

private:
    std::shared_ptr<DeviceService> m_deviceService;

    //! Coalesces the redraws. A knob drag emits a change per pixel of travel, and each one would
    //! otherwise pay for a render; restarting this on every change collapses a drag into one.
    QTimer m_waveformTimer;
    //! Redraws now rather than after the wait. Picking a voice is one deliberate act rather than a
    //! stream of them, and a quarter of a second of blank would read as the dialog being slow.
    void renderWaveform();
    //! \param delayMs How long to wait. The default coalesces a knob drag; zero defers the redraw
    //! to the next turn of the event loop without waiting, which is how a redraw is asked for
    //! without doing it inside the handler that asked.
    void scheduleWaveform(int delayMs = WaveformDebounceMs);

    //! Long enough that a knob drag settles into one render, short enough that letting go of the
    //! knob and looking at the picture feels like the same action.
    static constexpr int WaveformDebounceMs { 250 };

    double voiceAmpAttackSeconds() const;
    double voiceAmpHoldSeconds() const;
    double voiceAmpDecaySeconds() const;
    double voiceAmpReleaseSeconds() const;

    struct WaveformEnvelope
    {
        double attack { 0.0 };
        double hold { 0.0 };
        double decay { 0.0 };
        double sustain { 0.0 };
        double release { 0.0 };
        double curve { 0.0 };
    };
    WaveformEnvelope m_waveformEnvelope;

    QVariantList m_waveformData;
    double m_waveformDuration { 0.0 };
    double m_audibleLength { 0.0 };
    int m_waveformPeakCount { 0 };
    bool m_waveformVisible { false };
};

} // namespace noteahead

#endif // DRUM_PAD_CONTROLLER_HPP
