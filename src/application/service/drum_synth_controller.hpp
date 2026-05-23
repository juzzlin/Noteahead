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

#ifndef DRUM_SYNTH_CONTROLLER_HPP
#define DRUM_SYNTH_CONTROLLER_HPP

#include <QObject>
#include <memory>
#include <string>

namespace noteahead {

class DeviceService;
class DrumSynthDevice;

class DrumSynthController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int selectedVoice READ selectedVoice WRITE setSelectedVoice NOTIFY selectedVoiceChanged)

    // Selected Voice Parameters
    Q_PROPERTY(int voiceLevel READ voiceLevel WRITE setVoiceLevel NOTIFY voiceLevelChanged)
    Q_PROPERTY(int voicePan READ voicePan WRITE setVoicePan NOTIFY voicePanChanged)
    Q_PROPERTY(int voiceLpfCutoff READ voiceLpfCutoff WRITE setVoiceLpfCutoff NOTIFY voiceLpfCutoffChanged)
    Q_PROPERTY(int voiceHpfCutoff READ voiceHpfCutoff WRITE setVoiceHpfCutoff NOTIFY voiceHpfCutoffChanged)
    Q_PROPERTY(int voiceTune READ voiceTune WRITE setVoiceTune NOTIFY voiceTuneChanged)
    Q_PROPERTY(int voiceDecay READ voiceDecay WRITE setVoiceDecay NOTIFY voiceDecayChanged)
    Q_PROPERTY(int voiceAttack READ voiceAttack WRITE setVoiceAttack NOTIFY voiceAttackChanged)

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
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(int pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(uint32_t sampleRate READ sampleRate NOTIFY sampleRateChanged)

    // UI Helpers
    Q_PROPERTY(bool isKick READ isKick NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isSnare READ isSnare NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isTom READ isTom NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool isCymbal READ isCymbal NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool hasResonance READ hasResonance NOTIFY selectedVoiceChanged)
    Q_PROPERTY(bool hasAttack READ hasAttack NOTIFY selectedVoiceChanged)

public:
    explicit DrumSynthController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    Q_INVOKABLE void setDevice(const QString & deviceName);

    uint32_t sampleRate() const;
    Q_INVOKABLE float cutoffToHz(float cutoff) const;

    int selectedVoice() const;
    void setSelectedVoice(int index);

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

    int volume() const;
    void setVolume(int value);

    int gain() const;
    void setGain(int value);

    int pan() const;
    void setPan(int value);

    bool isKick() const;
    bool isSnare() const;
    bool isTom() const;
    bool isCymbal() const;
    bool hasResonance() const;
    bool hasAttack() const;

    Q_INVOKABLE void playNote(int note, double velocity = 1.0);
    Q_INVOKABLE void stopNote(int note);
    Q_INVOKABLE void playVoice(int index);

signals:
    void selectedVoiceChanged();
    void voiceLevelChanged();
    void voicePanChanged();
    void voiceLpfCutoffChanged();
    void voiceHpfCutoffChanged();
    void voiceTuneChanged();
    void voiceDecayChanged();
    void voiceAttackChanged();
    void kickAttackChanged();
    void kickClickTuneChanged();
    void kickPitchDepthChanged();
    void kickPitchDecayChanged();
    void snareSnappyChanged();
    void snareToneChanged();
    void tomPitchDepthChanged();
    void tomPitchDecayChanged();
    void voiceResonanceChanged();
    void volumeChanged();
    void gainChanged();
    void panChanged();
    void sampleRateChanged();

private:
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<DrumSynthDevice> m_device;
    int m_selectedVoice { 0 };

    std::string currentVoicePrefix() const;
    void updateProperties();
};

} // namespace noteahead

#endif // DRUM_SYNTH_CONTROLLER_HPP
