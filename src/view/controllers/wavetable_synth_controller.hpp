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

#ifndef WAVETABLE_SYNTH_CONTROLLER_HPP
#define WAVETABLE_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QStringList>
#include <memory>

namespace noteahead {

class DeviceService;
class WavetableSynthDevice;

class WavetableSynthController : public DeviceController
{
    Q_OBJECT

    // Osc 1
    Q_PROPERTY(int osc1Pos READ osc1Pos WRITE setOsc1Pos NOTIFY osc1PosChanged)
    Q_PROPERTY(int osc1Octave READ osc1Octave WRITE setOsc1Octave NOTIFY osc1OctaveChanged)
    Q_PROPERTY(int osc1Pitch READ osc1Pitch WRITE setOsc1Pitch NOTIFY osc1PitchChanged)
    Q_PROPERTY(int osc1Level READ osc1Level WRITE setOsc1Level NOTIFY osc1LevelChanged)

    // Osc 2
    Q_PROPERTY(int osc2Pos READ osc2Pos WRITE setOsc2Pos NOTIFY osc2PosChanged)
    Q_PROPERTY(int osc2Octave READ osc2Octave WRITE setOsc2Octave NOTIFY osc2OctaveChanged)
    Q_PROPERTY(int osc2Pitch READ osc2Pitch WRITE setOsc2Pitch NOTIFY osc2PitchChanged)
    Q_PROPERTY(int osc2Level READ osc2Level WRITE setOsc2Level NOTIFY osc2LevelChanged)

    // Noise
    Q_PROPERTY(int noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY noiseLevelChanged)

    // Filter
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)

    // Amp EG
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY ampAttackChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY ampDecayChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY ampSustainChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY ampReleaseChanged)

    // Mod EG
    Q_PROPERTY(int modAttack READ modAttack WRITE setModAttack NOTIFY modAttackChanged)
    Q_PROPERTY(int modDecay READ modDecay WRITE setModDecay NOTIFY modDecayChanged)
    Q_PROPERTY(int modInt READ modInt WRITE setModInt NOTIFY modIntChanged)
    Q_PROPERTY(int modTarget READ modTarget WRITE setModTarget NOTIFY modTargetChanged)

    // LFO
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY lfoWaveformChanged)
    Q_PROPERTY(int lfoMode READ lfoMode WRITE setLfoMode NOTIFY lfoModeChanged)
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoInt READ lfoInt WRITE setLfoInt NOTIFY lfoIntChanged)
    Q_PROPERTY(int lfoTarget READ lfoTarget WRITE setLfoTarget NOTIFY lfoTargetChanged)

    // Global
    Q_PROPERTY(int voiceMode READ voiceMode WRITE setVoiceMode NOTIFY voiceModeChanged)
    Q_PROPERTY(int voiceDepth READ voiceDepth WRITE setVoiceDepth NOTIFY voiceDepthChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)
    Q_PROPERTY(int portamento READ portamento WRITE setPortamento NOTIFY portamentoChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY pitchBendRangeChanged)
    Q_PROPERTY(int wavetableIndex READ wavetableIndex WRITE setWavetableIndex NOTIFY wavetableIndexChanged)
    Q_PROPERTY(QStringList wavetableNames READ wavetableNames NOTIFY wavetableNamesChanged)
    Q_PROPERTY(QStringList voiceModes READ voiceModes CONSTANT)
    Q_PROPERTY(QStringList octaveNames READ octaveNames CONSTANT)
    Q_PROPERTY(QStringList modTargetNames READ modTargetNames CONSTANT)
    Q_PROPERTY(QStringList lfoWaveformNames READ lfoWaveformNames CONSTANT)
    Q_PROPERTY(QStringList lfoModeNames READ lfoModeNames CONSTANT)
    Q_PROPERTY(QStringList lfoTargetNames READ lfoTargetNames CONSTANT)

public:
    explicit WavetableSynthController(std::shared_ptr<WavetableSynthDevice> synth, QObject * parent = nullptr);
    ~WavetableSynthController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    // Accessors (Osc 1)
    int osc1Pos() const;
    void setOsc1Pos(int pos);
    int osc1Octave() const;
    void setOsc1Octave(int oct);
    int osc1Pitch() const;
    void setOsc1Pitch(int p);
    int osc1Level() const;
    void setOsc1Level(int lvl);

    // Accessors (Osc 2)
    int osc2Pos() const;
    void setOsc2Pos(int pos);
    int osc2Octave() const;
    void setOsc2Octave(int oct);
    int osc2Pitch() const;
    void setOsc2Pitch(int p);
    int osc2Level() const;
    void setOsc2Level(int lvl);

    // Noise
    int noiseLevel() const;
    void setNoiseLevel(int lvl);

    // Filter
    int lpfCutoff() const;
    void setLpfCutoff(int c);
    int lpfResonance() const;
    void setLpfResonance(int r);
    int hpfCutoff() const;
    void setHpfCutoff(int c);

    // Amp EG
    int ampAttack() const;
    void setAmpAttack(int a);
    int ampDecay() const;
    void setAmpDecay(int d);
    int ampSustain() const;
    void setAmpSustain(int s);
    int ampRelease() const;
    void setAmpRelease(int r);

    // Mod EG
    int modAttack() const;
    void setModAttack(int a);
    int modDecay() const;
    void setModDecay(int d);
    int modInt() const;
    void setModInt(int i);
    int modTarget() const;
    void setModTarget(int t);

    // LFO
    int lfoWaveform() const;
    void setLfoWaveform(int wave);
    int lfoMode() const;
    void setLfoMode(int mode);
    int lfoRate() const;
    void setLfoRate(int rate);
    int lfoInt() const;
    void setLfoInt(int i);
    int lfoTarget() const;
    void setLfoTarget(int target);

    // Global
    int voiceMode() const;
    void setVoiceMode(int mode);
    int voiceDepth() const;
    void setVoiceDepth(int depth);
    int panSpread() const;
    void setPanSpread(int spread);
    int portamento() const;
    void setPortamento(int p);
    int pitchBendRange() const;
    void setPitchBendRange(int range);

    int wavetableIndex() const;
    void setWavetableIndex(int index);
    QStringList wavetableNames() const;
    QStringList voiceModes() const;
    QStringList octaveNames() const;
    QStringList modTargetNames() const;
    QStringList lfoWaveformNames() const;
    QStringList lfoModeNames() const;
    QStringList lfoTargetNames() const;

    Q_INVOKABLE void requestSettings() override;

    void setDeviceService(std::shared_ptr<DeviceService> deviceService);

signals:
    void osc1PosChanged();
    void osc1OctaveChanged();
    void osc1PitchChanged();
    void osc1LevelChanged();
    void osc2PosChanged();
    void osc2OctaveChanged();
    void osc2PitchChanged();
    void osc2LevelChanged();
    void noiseLevelChanged();
    void lpfCutoffChanged();
    void lpfResonanceChanged();
    void hpfCutoffChanged();
    void ampAttackChanged();
    void ampDecayChanged();
    void ampSustainChanged();
    void ampReleaseChanged();
    void modAttackChanged();
    void modDecayChanged();
    void modIntChanged();
    void modTargetChanged();
    void lfoWaveformChanged();
    void lfoModeChanged();
    void lfoRateChanged();
    void lfoIntChanged();
    void lfoTargetChanged();
    void voiceModeChanged();
    void voiceDepthChanged();
    void panSpreadChanged();
    void portamentoChanged();
    void pitchBendRangeChanged();
    void wavetableIndexChanged();
    void wavetableNamesChanged();

private:
    std::shared_ptr<WavetableSynthDevice> m_synth;
    std::shared_ptr<DeviceService> m_deviceService;

    void connectDeviceSignals();
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_CONTROLLER_HPP
