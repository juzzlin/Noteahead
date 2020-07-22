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

    Q_PROPERTY(int selectedPad READ selectedPad WRITE setSelectedPad NOTIFY selectedPadChanged)
    
    // Selected Pad Parameters
    Q_PROPERTY(int padLevel READ padLevel WRITE setPadLevel NOTIFY padLevelChanged)
    Q_PROPERTY(int padPan READ padPan WRITE setPadPan NOTIFY padPanChanged)
    Q_PROPERTY(int padLpfCutoff READ padLpfCutoff WRITE setPadLpfCutoff NOTIFY padLpfCutoffChanged)
    Q_PROPERTY(int padHpfCutoff READ padHpfCutoff WRITE setPadHpfCutoff NOTIFY padHpfCutoffChanged)
    Q_PROPERTY(int padTune READ padTune WRITE setPadTune NOTIFY padTuneChanged)
    Q_PROPERTY(int padDecay READ padDecay WRITE setPadDecay NOTIFY padDecayChanged)
    Q_PROPERTY(int padAttack READ padAttack WRITE setPadAttack NOTIFY padAttackChanged)
    
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
    Q_PROPERTY(int padResonance READ padResonance WRITE setPadResonance NOTIFY padResonanceChanged)

    // Global
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(int pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(uint32_t sampleRate READ sampleRate NOTIFY sampleRateChanged)

public:
    explicit DrumSynthController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    Q_INVOKABLE void setDevice(const QString & deviceName);

    uint32_t sampleRate() const;
    Q_INVOKABLE float cutoffToHz(float cutoff) const;

    int selectedPad() const;
    void setSelectedPad(int index);

    int padLevel() const;
    void setPadLevel(int value);

    int padPan() const;
    void setPadPan(int value);

    int padLpfCutoff() const;
    void setPadLpfCutoff(int value);

    int padHpfCutoff() const;
    void setPadHpfCutoff(int value);

    int padTune() const;
    void setPadTune(int value);

    int padDecay() const;
    void setPadDecay(int value);

    int padAttack() const;
    void setPadAttack(int value);

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

    int padResonance() const;
    void setPadResonance(int value);

    int volume() const;
    void setVolume(int value);

    int gain() const;
    void setGain(int value);

    int pan() const;
    void setPan(int value);

    Q_INVOKABLE void playNote(int note, double velocity = 1.0);
    Q_INVOKABLE void stopNote(int note);
    Q_INVOKABLE void playPad(int index);

signals:
    void selectedPadChanged();
    void padLevelChanged();
    void padPanChanged();
    void padLpfCutoffChanged();
    void padHpfCutoffChanged();
    void padTuneChanged();
    void padDecayChanged();
    void padAttackChanged();
    void kickAttackChanged();
    void kickClickTuneChanged();
    void kickPitchDepthChanged();
    void kickPitchDecayChanged();
    void snareSnappyChanged();
    void snareToneChanged();
    void tomPitchDepthChanged();
    void tomPitchDecayChanged();
    void padResonanceChanged();
    void volumeChanged();
    void gainChanged();
    void panChanged();
    void sampleRateChanged();

private:
    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<DrumSynthDevice> m_device;
    int m_selectedPad { 0 };

    std::string currentPadPrefix() const;
    void updateProperties();
};

} // namespace noteahead

#endif // DRUM_SYNTH_CONTROLLER_HPP
