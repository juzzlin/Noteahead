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

#ifndef VIEW_CONTROLLERS_JU_SYNTH_08_CONTROLLER_HPP
#define VIEW_CONTROLLERS_JU_SYNTH_08_CONTROLLER_HPP

#include "device_controller.hpp"

namespace noteahead {

class JuSynth08Device;

class JuSynth08Controller : public DeviceController
{
    Q_OBJECT

    // LFO
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoDelay READ lfoDelay WRITE setLfoDelay NOTIFY lfoDelayChanged)
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY lfoWaveformChanged)

    // VCO Mod & Pitch
    Q_PROPERTY(int pitchLfoDepth READ pitchLfoDepth WRITE setPitchLfoDepth NOTIFY pitchLfoDepthChanged)
    Q_PROPERTY(int pitchEnvDepth READ pitchEnvDepth WRITE setPitchEnvDepth NOTIFY pitchEnvDepthChanged)
    Q_PROPERTY(int pitchEnvTarget READ pitchEnvTarget WRITE setPitchEnvTarget NOTIFY pitchEnvTargetChanged)
    Q_PROPERTY(int pulseWidth READ pulseWidth WRITE setPulseWidth NOTIFY pulseWidthChanged)
    Q_PROPERTY(int pwmSource READ pwmSource WRITE setPwmSource NOTIFY pwmSourceChanged)
    Q_PROPERTY(int crossMod READ crossMod WRITE setCrossMod NOTIFY crossModChanged)

    // VCO 1
    Q_PROPERTY(int vco1Waveform READ vco1Waveform WRITE setVco1Waveform NOTIFY vco1WaveformChanged)
    Q_PROPERTY(int vco1Range READ vco1Range WRITE setVco1Range NOTIFY vco1RangeChanged)
    Q_PROPERTY(int vco1Level READ vco1Level WRITE setVco1Level NOTIFY vco1LevelChanged)

    // VCO 2
    Q_PROPERTY(int vco2Waveform READ vco2Waveform WRITE setVco2Waveform NOTIFY vco2WaveformChanged)
    Q_PROPERTY(int vco2Range READ vco2Range WRITE setVco2Range NOTIFY vco2RangeChanged)
    Q_PROPERTY(int vco2FineTune READ vco2FineTune WRITE setVco2FineTune NOTIFY vco2FineTuneChanged)
    Q_PROPERTY(bool sync READ sync WRITE setSync NOTIFY syncChanged)
    Q_PROPERTY(int vco2Level READ vco2Level WRITE setVco2Level NOTIFY vco2LevelChanged)

    // Filter & Noise
    Q_PROPERTY(int noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY noiseLevelChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int vcfSlope READ vcfSlope WRITE setVcfSlope NOTIFY vcfSlopeChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int envMod READ envMod WRITE setEnvMod NOTIFY envModChanged)
    Q_PROPERTY(bool envPolarity READ envPolarity WRITE setEnvPolarity NOTIFY envPolarityChanged)
    Q_PROPERTY(int filterLfoDepth READ filterLfoDepth WRITE setFilterLfoDepth NOTIFY filterLfoDepthChanged)
    Q_PROPERTY(int keyTrack READ keyTrack WRITE setKeyTrack NOTIFY keyTrackChanged)

    // VCA
    Q_PROPERTY(int vcaMode READ vcaMode WRITE setVcaMode NOTIFY vcaModeChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)
    Q_PROPERTY(int vcaLfoDepth READ vcaLfoDepth WRITE setVcaLfoDepth NOTIFY vcaLfoDepthChanged)

    // ENV 1
    Q_PROPERTY(int env1Attack READ env1Attack WRITE setEnv1Attack NOTIFY env1AttackChanged)
    Q_PROPERTY(int env1Decay READ env1Decay WRITE setEnv1Decay NOTIFY env1DecayChanged)
    Q_PROPERTY(int env1Sustain READ env1Sustain WRITE setEnv1Sustain NOTIFY env1SustainChanged)
    Q_PROPERTY(int env1Release READ env1Release WRITE setEnv1Release NOTIFY env1ReleaseChanged)
    Q_PROPERTY(bool env1KeyFollow READ env1KeyFollow WRITE setEnv1KeyFollow NOTIFY env1KeyFollowChanged)

    // ENV 2
    Q_PROPERTY(int env2Attack READ env2Attack WRITE setEnv2Attack NOTIFY env2AttackChanged)
    Q_PROPERTY(int env2Decay READ env2Decay WRITE setEnv2Decay NOTIFY env2DecayChanged)
    Q_PROPERTY(int env2Sustain READ env2Sustain WRITE setEnv2Sustain NOTIFY env2SustainChanged)
    Q_PROPERTY(int env2Release READ env2Release WRITE setEnv2Release NOTIFY env2ReleaseChanged)
    Q_PROPERTY(bool env2KeyFollow READ env2KeyFollow WRITE setEnv2KeyFollow NOTIFY env2KeyFollowChanged)

    // Delay
    Q_PROPERTY(int delayTime READ delayTime WRITE setDelayTime NOTIFY delayTimeChanged)
    Q_PROPERTY(int delayFeedback READ delayFeedback WRITE setDelayFeedback NOTIFY delayFeedbackChanged)
    Q_PROPERTY(int delayLevel READ delayLevel WRITE setDelayLevel NOTIFY delayLevelChanged)

    // Voice & Global
    Q_PROPERTY(int voiceMode READ voiceMode WRITE setVoiceMode NOTIFY voiceModeChanged)
    Q_PROPERTY(int unisonDetune READ unisonDetune WRITE setUnisonDetune NOTIFY unisonDetuneChanged)
    Q_PROPERTY(int portamento READ portamento WRITE setPortamento NOTIFY portamentoChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY pitchBendRangeChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)

public:
    explicit JuSynth08Controller(std::shared_ptr<JuSynth08Device> device = nullptr, QObject * parent = nullptr);
    ~JuSynth08Controller() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    void setDevice(std::shared_ptr<JuSynth08Device> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    // LFO
    int lfoRate() const;
    void setLfoRate(int value);

    int lfoDelay() const;
    void setLfoDelay(int value);

    int lfoWaveform() const;
    void setLfoWaveform(int value);

    // VCO Mod & Pitch
    int pitchLfoDepth() const;
    void setPitchLfoDepth(int value);

    int pitchEnvDepth() const;
    void setPitchEnvDepth(int value);

    int pitchEnvTarget() const;
    void setPitchEnvTarget(int value);

    int pulseWidth() const;
    void setPulseWidth(int value);

    int pwmSource() const;
    void setPwmSource(int value);

    int crossMod() const;
    void setCrossMod(int value);

    // VCO 1
    int vco1Waveform() const;
    void setVco1Waveform(int value);

    int vco1Range() const;
    void setVco1Range(int value);

    int vco1Level() const;
    void setVco1Level(int value);

    // VCO 2
    int vco2Waveform() const;
    void setVco2Waveform(int value);

    int vco2Range() const;
    void setVco2Range(int value);

    int vco2FineTune() const;
    void setVco2FineTune(int value);

    bool sync() const;
    void setSync(bool value);

    int vco2Level() const;
    void setVco2Level(int value);

    // Filter & Noise
    int noiseLevel() const;
    void setNoiseLevel(int value);

    int hpfCutoff() const;
    void setHpfCutoff(int value);

    int vcfSlope() const;
    void setVcfSlope(int value);

    int lpfCutoff() const;
    void setLpfCutoff(int value);

    int lpfResonance() const;
    void setLpfResonance(int value);

    int envMod() const;
    void setEnvMod(int value);

    bool envPolarity() const;
    void setEnvPolarity(bool value);

    int filterLfoDepth() const;
    void setFilterLfoDepth(int value);

    int keyTrack() const;
    void setKeyTrack(int value);

    // VCA
    int vcaMode() const;
    void setVcaMode(int value);

    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);

    int vcaLfoDepth() const;
    void setVcaLfoDepth(int value);

    // ENV 1
    int env1Attack() const;
    void setEnv1Attack(int value);

    int env1Decay() const;
    void setEnv1Decay(int value);

    int env1Sustain() const;
    void setEnv1Sustain(int value);

    int env1Release() const;
    void setEnv1Release(int value);

    bool env1KeyFollow() const;
    void setEnv1KeyFollow(bool value);

    // ENV 2
    int env2Attack() const;
    void setEnv2Attack(int value);

    int env2Decay() const;
    void setEnv2Decay(int value);

    int env2Sustain() const;
    void setEnv2Sustain(int value);

    int env2Release() const;
    void setEnv2Release(int value);

    bool env2KeyFollow() const;
    void setEnv2KeyFollow(bool value);

    // Delay
    int delayTime() const;
    void setDelayTime(int value);

    int delayFeedback() const;
    void setDelayFeedback(int value);

    int delayLevel() const;
    void setDelayLevel(int value);

    // Global
    int voiceMode() const;
    void setVoiceMode(int value);

    int unisonDetune() const;
    void setUnisonDetune(int value);

    int portamento() const;
    void setPortamento(int value);

    int pitchBendRange() const;
    void setPitchBendRange(int value);

    int panSpread() const;
    void setPanSpread(int value);

public slots:
    void requestSettings() override;

signals:
    void deviceChanged();

    void lfoRateChanged();
    void lfoDelayChanged();
    void lfoWaveformChanged();

    void pitchLfoDepthChanged();
    void pitchEnvDepthChanged();
    void pitchEnvTargetChanged();
    void pulseWidthChanged();
    void pwmSourceChanged();
    void crossModChanged();

    void vco1WaveformChanged();
    void vco1RangeChanged();
    void vco1LevelChanged();

    void vco2WaveformChanged();
    void vco2RangeChanged();
    void vco2FineTuneChanged();
    void syncChanged();
    void vco2LevelChanged();

    void noiseLevelChanged();
    void hpfCutoffChanged();

    void vcfSlopeChanged();
    void lpfCutoffChanged();
    void lpfResonanceChanged();
    void envModChanged();
    void envPolarityChanged();
    void filterLfoDepthChanged();
    void keyTrackChanged();

    void vcaModeChanged();
    void velocitySensitivityChanged();
    void vcaLfoDepthChanged();

    void env1AttackChanged();
    void env1DecayChanged();
    void env1SustainChanged();
    void env1ReleaseChanged();
    void env1KeyFollowChanged();

    void env2AttackChanged();
    void env2DecayChanged();
    void env2SustainChanged();
    void env2ReleaseChanged();
    void env2KeyFollowChanged();

    void delayTimeChanged();
    void delayFeedbackChanged();
    void delayLevelChanged();

    void voiceModeChanged();
    void unisonDetuneChanged();
    void portamentoChanged();
    void pitchBendRangeChanged();
    void panSpreadChanged();

private:
    std::shared_ptr<JuSynth08Device> m_device;
};

} // namespace noteahead

#endif // VIEW_CONTROLLERS_JU_SYNTH_08_CONTROLLER_HPP
