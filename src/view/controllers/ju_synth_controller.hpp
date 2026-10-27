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

#ifndef JU_SYNTH_CONTROLLER_HPP
#define JU_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"

#include <memory>

namespace noteahead {

class JuSynthDevice;

//! The JU Synth's panel. Continuous controls are scaled by Constants::uiInternalScaling(); the
//! switches are bools and the selectors their index.
class JuSynthController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoDelay READ lfoDelay WRITE setLfoDelay NOTIFY lfoDelayChanged)
    Q_PROPERTY(int pitchLfoDepth READ pitchLfoDepth WRITE setPitchLfoDepth NOTIFY pitchLfoDepthChanged)
    Q_PROPERTY(int pulseWidth READ pulseWidth WRITE setPulseWidth NOTIFY pulseWidthChanged)
    Q_PROPERTY(int subLevel READ subLevel WRITE setSubLevel NOTIFY subLevelChanged)
    Q_PROPERTY(int noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY noiseLevelChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int envMod READ envMod WRITE setEnvMod NOTIFY envModChanged)
    Q_PROPERTY(int filterLfoDepth READ filterLfoDepth WRITE setFilterLfoDepth NOTIFY filterLfoDepthChanged)
    Q_PROPERTY(int keyTrack READ keyTrack WRITE setKeyTrack NOTIFY keyTrackChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)
    Q_PROPERTY(int attack READ attack WRITE setAttack NOTIFY attackChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int sustain READ sustain WRITE setSustain NOTIFY sustainChanged)
    Q_PROPERTY(int release READ release WRITE setRelease NOTIFY releaseChanged)
    Q_PROPERTY(int portamento READ portamento WRITE setPortamento NOTIFY portamentoChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)
    Q_PROPERTY(int oscillatorDrift READ oscillatorDrift WRITE setOscillatorDrift NOTIFY oscillatorDriftChanged)
    Q_PROPERTY(int voiceTolerance READ voiceTolerance WRITE setVoiceTolerance NOTIFY voiceToleranceChanged)
    Q_PROPERTY(int range READ range WRITE setRange NOTIFY rangeChanged)
    Q_PROPERTY(int pwmSource READ pwmSource WRITE setPwmSource NOTIFY pwmSourceChanged)
    Q_PROPERTY(int vcaMode READ vcaMode WRITE setVcaMode NOTIFY vcaModeChanged)
    Q_PROPERTY(int chorusMode READ chorusMode WRITE setChorusMode NOTIFY chorusModeChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY pitchBendRangeChanged)
    Q_PROPERTY(bool sawEnabled READ sawEnabled WRITE setSawEnabled NOTIFY sawEnabledChanged)
    Q_PROPERTY(bool pulseEnabled READ pulseEnabled WRITE setPulseEnabled NOTIFY pulseEnabledChanged)
    Q_PROPERTY(bool envInverted READ envInverted WRITE setEnvInverted NOTIFY envInvertedChanged)

public:
    explicit JuSynthController(std::shared_ptr<JuSynthDevice> device, QObject * parent = nullptr);
    ~JuSynthController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    void setDevice(std::shared_ptr<JuSynthDevice> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int lfoRate() const;
    void setLfoRate(int value);
    int lfoDelay() const;
    void setLfoDelay(int value);
    int pitchLfoDepth() const;
    void setPitchLfoDepth(int value);
    int pulseWidth() const;
    void setPulseWidth(int value);
    int subLevel() const;
    void setSubLevel(int value);
    int noiseLevel() const;
    void setNoiseLevel(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int lpfResonance() const;
    void setLpfResonance(int value);
    int envMod() const;
    void setEnvMod(int value);
    int filterLfoDepth() const;
    void setFilterLfoDepth(int value);
    int keyTrack() const;
    void setKeyTrack(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int attack() const;
    void setAttack(int value);
    int decay() const;
    void setDecay(int value);
    int sustain() const;
    void setSustain(int value);
    int release() const;
    void setRelease(int value);
    int portamento() const;
    void setPortamento(int value);
    int panSpread() const;
    void setPanSpread(int value);
    int oscillatorDrift() const;
    void setOscillatorDrift(int value);
    int voiceTolerance() const;
    void setVoiceTolerance(int value);
    int range() const;
    void setRange(int value);
    int pwmSource() const;
    void setPwmSource(int value);
    int vcaMode() const;
    void setVcaMode(int value);
    int chorusMode() const;
    void setChorusMode(int value);
    int pitchBendRange() const;
    void setPitchBendRange(int value);
    bool sawEnabled() const;
    void setSawEnabled(bool value);
    bool pulseEnabled() const;
    void setPulseEnabled(bool value);
    bool envInverted() const;
    void setEnvInverted(bool value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void lfoRateChanged();
    void lfoDelayChanged();
    void pitchLfoDepthChanged();
    void pulseWidthChanged();
    void subLevelChanged();
    void noiseLevelChanged();
    void hpfCutoffChanged();
    void lpfCutoffChanged();
    void lpfResonanceChanged();
    void envModChanged();
    void filterLfoDepthChanged();
    void keyTrackChanged();
    void velocitySensitivityChanged();
    void attackChanged();
    void decayChanged();
    void sustainChanged();
    void releaseChanged();
    void portamentoChanged();
    void panSpreadChanged();
    void oscillatorDriftChanged();
    void voiceToleranceChanged();
    void rangeChanged();
    void pwmSourceChanged();
    void vcaModeChanged();
    void chorusModeChanged();
    void pitchBendRangeChanged();
    void sawEnabledChanged();
    void pulseEnabledChanged();
    void envInvertedChanged();

private:
    std::shared_ptr<JuSynthDevice> m_device;
};

} // namespace noteahead

#endif // JU_SYNTH_CONTROLLER_HPP
