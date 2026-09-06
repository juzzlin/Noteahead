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

#ifndef FM_SYNTH_CONTROLLER_HPP
#define FM_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QStringList>
#include <QVariantList>

#include <memory>
#include <vector>

namespace noteahead {

class DeviceService;
class FmOperatorController;
class FmSynthDevice;

class FmSynthController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int algorithm READ algorithm WRITE setAlgorithm NOTIFY algorithmChanged)
    Q_PROPERTY(int feedback READ feedback WRITE setFeedback NOTIFY feedbackChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY ampAttackChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY ampDecayChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY ampSustainChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY ampReleaseChanged)
    Q_PROPERTY(int ampCurve READ ampCurve WRITE setAmpCurve NOTIFY ampCurveChanged)
    Q_PROPERTY(int ampVelocitySensitivity READ ampVelocitySensitivity WRITE setAmpVelocitySensitivity NOTIFY ampVelocitySensitivityChanged)
    Q_PROPERTY(int modAttack READ modAttack WRITE setModAttack NOTIFY modAttackChanged)
    Q_PROPERTY(int modDecay READ modDecay WRITE setModDecay NOTIFY modDecayChanged)
    Q_PROPERTY(int modSustain READ modSustain WRITE setModSustain NOTIFY modSustainChanged)
    Q_PROPERTY(int modInt READ modInt WRITE setModInt NOTIFY modIntChanged)
    Q_PROPERTY(int modTarget READ modTarget WRITE setModTarget NOTIFY modTargetChanged)
    Q_PROPERTY(int modCurve READ modCurve WRITE setModCurve NOTIFY modCurveChanged)
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY lfoWaveformChanged)
    Q_PROPERTY(int lfoMode READ lfoMode WRITE setLfoMode NOTIFY lfoModeChanged)
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoInt READ lfoInt WRITE setLfoInt NOTIFY lfoIntChanged)
    Q_PROPERTY(int lfoTarget READ lfoTarget WRITE setLfoTarget NOTIFY lfoTargetChanged)
    Q_PROPERTY(int lfo2Waveform READ lfo2Waveform WRITE setLfo2Waveform NOTIFY lfo2WaveformChanged)
    Q_PROPERTY(int lfo2Mode READ lfo2Mode WRITE setLfo2Mode NOTIFY lfo2ModeChanged)
    Q_PROPERTY(int lfo2Rate READ lfo2Rate WRITE setLfo2Rate NOTIFY lfo2RateChanged)
    Q_PROPERTY(int lfo2Int READ lfo2Int WRITE setLfo2Int NOTIFY lfo2IntChanged)
    Q_PROPERTY(int lfo2Target READ lfo2Target WRITE setLfo2Target NOTIFY lfo2TargetChanged)
    Q_PROPERTY(int voiceMode READ voiceMode WRITE setVoiceMode NOTIFY voiceModeChanged)
    Q_PROPERTY(int voiceDepth READ voiceDepth WRITE setVoiceDepth NOTIFY voiceDepthChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)
    Q_PROPERTY(int portamento READ portamento WRITE setPortamento NOTIFY portamentoChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY pitchBendRangeChanged)

    //! The four operator panels, in order. Constant: the objects live as long as the controller and
    //! follow whichever device it is pointed at.
    Q_PROPERTY(QVariantList operators READ operators CONSTANT)

    Q_PROPERTY(QStringList algorithmNames READ algorithmNames CONSTANT)
    Q_PROPERTY(QStringList operatorWaveformNames READ operatorWaveformNames CONSTANT)
    Q_PROPERTY(QStringList ratioNames READ ratioNames CONSTANT)
    Q_PROPERTY(QStringList voiceModes READ voiceModes NOTIFY translationsChanged)
    Q_PROPERTY(QStringList modTargetNames READ modTargetNames NOTIFY translationsChanged)
    Q_PROPERTY(QStringList lfoWaveformNames READ lfoWaveformNames CONSTANT)
    Q_PROPERTY(QStringList lfoModeNames READ lfoModeNames NOTIFY translationsChanged)
    Q_PROPERTY(QStringList lfoTargetNames READ lfoTargetNames NOTIFY translationsChanged)

public:
    explicit FmSynthController(std::shared_ptr<FmSynthDevice> synth, QObject * parent = nullptr);
    ~FmSynthController() override;

    //! Re-emits the notifications for everything holding translated strings.
    void retranslate();

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int algorithm() const;
    void setAlgorithm(int value);
    int feedback() const;
    void setFeedback(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int lpfResonance() const;
    void setLpfResonance(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    int ampAttack() const;
    void setAmpAttack(int value);
    int ampDecay() const;
    void setAmpDecay(int value);
    int ampSustain() const;
    void setAmpSustain(int value);
    int ampRelease() const;
    void setAmpRelease(int value);
    int ampCurve() const;
    void setAmpCurve(int value);
    int ampVelocitySensitivity() const;
    void setAmpVelocitySensitivity(int value);
    int modAttack() const;
    void setModAttack(int value);
    int modDecay() const;
    void setModDecay(int value);
    int modSustain() const;
    void setModSustain(int value);
    int modInt() const;
    void setModInt(int value);
    int modTarget() const;
    void setModTarget(int value);
    int modCurve() const;
    void setModCurve(int value);
    int lfoWaveform() const;
    void setLfoWaveform(int value);
    int lfoMode() const;
    void setLfoMode(int value);
    int lfoRate() const;
    void setLfoRate(int value);
    int lfoInt() const;
    void setLfoInt(int value);
    int lfoTarget() const;
    void setLfoTarget(int value);
    int lfo2Waveform() const;
    void setLfo2Waveform(int value);
    int lfo2Mode() const;
    void setLfo2Mode(int value);
    int lfo2Rate() const;
    void setLfo2Rate(int value);
    int lfo2Int() const;
    void setLfo2Int(int value);
    int lfo2Target() const;
    void setLfo2Target(int value);
    int voiceMode() const;
    void setVoiceMode(int value);
    int voiceDepth() const;
    void setVoiceDepth(int value);
    int panSpread() const;
    void setPanSpread(int value);
    int portamento() const;
    void setPortamento(int value);
    int pitchBendRange() const;
    void setPitchBendRange(int value);

    QVariantList operators() const;

    QStringList algorithmNames() const;
    QStringList operatorWaveformNames() const;
    QStringList ratioNames() const;
    QStringList voiceModes() const;
    QStringList modTargetNames() const;
    QStringList lfoWaveformNames() const;
    QStringList lfoModeNames() const;
    QStringList lfoTargetNames() const;

    Q_INVOKABLE void requestSettings() override;

    void setDeviceService(std::shared_ptr<DeviceService> deviceService);

signals:
    void translationsChanged();

    void algorithmChanged();
    void feedbackChanged();
    void lpfCutoffChanged();
    void lpfResonanceChanged();
    void hpfCutoffChanged();
    void ampAttackChanged();
    void ampDecayChanged();
    void ampSustainChanged();
    void ampReleaseChanged();
    void ampCurveChanged();
    void ampVelocitySensitivityChanged();
    void modAttackChanged();
    void modDecayChanged();
    void modSustainChanged();
    void modIntChanged();
    void modTargetChanged();
    void modCurveChanged();
    void lfoWaveformChanged();
    void lfoModeChanged();
    void lfoRateChanged();
    void lfoIntChanged();
    void lfoTargetChanged();
    void lfo2WaveformChanged();
    void lfo2ModeChanged();
    void lfo2RateChanged();
    void lfo2IntChanged();
    void lfo2TargetChanged();
    void voiceModeChanged();
    void voiceDepthChanged();
    void panSpreadChanged();
    void portamentoChanged();
    void pitchBendRangeChanged();

private:
    std::shared_ptr<FmSynthDevice> m_synth;
    std::shared_ptr<DeviceService> m_deviceService;
    std::vector<FmOperatorController *> m_operators;

    void connectDeviceSignals();
};

} // namespace noteahead

#endif // FM_SYNTH_CONTROLLER_HPP
