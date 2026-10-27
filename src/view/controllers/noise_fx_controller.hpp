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

#ifndef NOISE_FX_CONTROLLER_HPP
#define NOISE_FX_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QStringList>
#include <QVariantList>

#include <memory>
#include <vector>

namespace noteahead {

class NoiseFxDevice;
class NoiseFxSweepController;

//! The Noise FX panel. Continuous controls are scaled by Constants::uiInternalScaling(); the
//! switches are bools and the selectors their index. The two sweeps are panels of their own.
class NoiseFxController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int colour READ colour WRITE setColour NOTIFY colourChanged)
    Q_PROPERTY(int sampleHoldRate READ sampleHoldRate WRITE setSampleHoldRate NOTIFY sampleHoldRateChanged)
    Q_PROPERTY(int crackle READ crackle WRITE setCrackle NOTIFY crackleChanged)
    Q_PROPERTY(int noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY noiseLevelChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int toneWaveform READ toneWaveform WRITE setToneWaveform NOTIFY toneWaveformChanged)
    Q_PROPERTY(int pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(int blend READ blend WRITE setBlend NOTIFY blendChanged)
    Q_PROPERTY(int ringMod READ ringMod WRITE setRingMod NOTIFY ringModChanged)
    Q_PROPERTY(int filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(int cutoff READ cutoff WRITE setCutoff NOTIFY cutoffChanged)
    Q_PROPERTY(int resonance READ resonance WRITE setResonance NOTIFY resonanceChanged)
    Q_PROPERTY(int drive READ drive WRITE setDrive NOTIFY driveChanged)
    Q_PROPERTY(int keyTrack READ keyTrack WRITE setKeyTrack NOTIFY keyTrackChanged)
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY ampAttackChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY ampDecayChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY ampSustainChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY ampReleaseChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)
    Q_PROPERTY(bool playToEnd READ playToEnd WRITE setPlayToEnd NOTIFY playToEndChanged)
    Q_PROPERTY(bool chromatic READ chromatic WRITE setChromatic NOTIFY chromaticChanged)
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY lfoWaveformChanged)
    Q_PROPERTY(bool lfoSync READ lfoSync WRITE setLfoSync NOTIFY lfoSyncChanged)
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoTarget1 READ lfoTarget1 WRITE setLfoTarget1 NOTIFY lfoTarget1Changed)
    Q_PROPERTY(int lfoDepth1 READ lfoDepth1 WRITE setLfoDepth1 NOTIFY lfoDepth1Changed)
    Q_PROPERTY(int lfoTarget2 READ lfoTarget2 WRITE setLfoTarget2 NOTIFY lfoTarget2Changed)
    Q_PROPERTY(int lfoDepth2 READ lfoDepth2 WRITE setLfoDepth2 NOTIFY lfoDepth2Changed)
    Q_PROPERTY(int gateRate READ gateRate WRITE setGateRate NOTIFY gateRateChanged)
    Q_PROPERTY(int gateDepth READ gateDepth WRITE setGateDepth NOTIFY gateDepthChanged)
    Q_PROPERTY(int gateWidth READ gateWidth WRITE setGateWidth NOTIFY gateWidthChanged)
    Q_PROPERTY(int gateSmoothing READ gateSmoothing WRITE setGateSmoothing NOTIFY gateSmoothingChanged)
    //! The synced LFO's division, an index into lfoSyncNames: the rate knob's position read as one.
    Q_PROPERTY(int lfoSyncDivision READ lfoSyncDivision WRITE setLfoSyncDivision NOTIFY lfoRateChanged)

    //! Sweep A and Sweep B. Constant: the objects live as long as the controller.
    Q_PROPERTY(QVariantList sweeps READ sweeps CONSTANT)
    //! Note values, the way the tracker's tempo divisions read: "1/16" to "32/1" bars and beats.
    Q_PROPERTY(QStringList syncLengthNames READ syncLengthNames CONSTANT)
    Q_PROPERTY(QStringList gateDivisionNames READ gateDivisionNames CONSTANT)
    Q_PROPERTY(QStringList lfoSyncNames READ lfoSyncNames CONSTANT)

public:
    explicit NoiseFxController(std::shared_ptr<NoiseFxDevice> device, QObject * parent = nullptr);
    ~NoiseFxController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;
    void setDevice(std::shared_ptr<NoiseFxDevice> device);

    //! The device the panels read and write, or nothing.
    std::shared_ptr<NoiseFxDevice> noiseFxDevice() const;

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int colour() const;
    void setColour(int value);
    int sampleHoldRate() const;
    void setSampleHoldRate(int value);
    int crackle() const;
    void setCrackle(int value);
    int noiseLevel() const;
    void setNoiseLevel(int value);
    int width() const;
    void setWidth(int value);
    int toneWaveform() const;
    void setToneWaveform(int value);
    int pitch() const;
    void setPitch(int value);
    int blend() const;
    void setBlend(int value);
    int ringMod() const;
    void setRingMod(int value);
    int filterType() const;
    void setFilterType(int value);
    int cutoff() const;
    void setCutoff(int value);
    int resonance() const;
    void setResonance(int value);
    int drive() const;
    void setDrive(int value);
    int keyTrack() const;
    void setKeyTrack(int value);
    int ampAttack() const;
    void setAmpAttack(int value);
    int ampDecay() const;
    void setAmpDecay(int value);
    int ampSustain() const;
    void setAmpSustain(int value);
    int ampRelease() const;
    void setAmpRelease(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    bool playToEnd() const;
    void setPlayToEnd(bool value);
    bool chromatic() const;
    void setChromatic(bool value);
    int lfoWaveform() const;
    void setLfoWaveform(int value);
    bool lfoSync() const;
    void setLfoSync(bool value);
    int lfoRate() const;
    void setLfoRate(int value);
    int lfoTarget1() const;
    void setLfoTarget1(int value);
    int lfoDepth1() const;
    void setLfoDepth1(int value);
    int lfoTarget2() const;
    void setLfoTarget2(int value);
    int lfoDepth2() const;
    void setLfoDepth2(int value);
    int gateRate() const;
    void setGateRate(int value);
    int gateDepth() const;
    void setGateDepth(int value);
    int gateWidth() const;
    void setGateWidth(int value);
    int gateSmoothing() const;
    void setGateSmoothing(int value);
    int lfoSyncDivision() const;
    void setLfoSyncDivision(int value);

    QVariantList sweeps() const;
    QStringList syncLengthNames() const;
    QStringList gateDivisionNames() const;
    QStringList lfoSyncNames() const;

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void colourChanged();
    void sampleHoldRateChanged();
    void crackleChanged();
    void noiseLevelChanged();
    void widthChanged();
    void toneWaveformChanged();
    void pitchChanged();
    void blendChanged();
    void ringModChanged();
    void filterTypeChanged();
    void cutoffChanged();
    void resonanceChanged();
    void driveChanged();
    void keyTrackChanged();
    void ampAttackChanged();
    void ampDecayChanged();
    void ampSustainChanged();
    void ampReleaseChanged();
    void velocitySensitivityChanged();
    void playToEndChanged();
    void chromaticChanged();
    void lfoWaveformChanged();
    void lfoSyncChanged();
    void lfoRateChanged();
    void lfoTarget1Changed();
    void lfoDepth1Changed();
    void lfoTarget2Changed();
    void lfoDepth2Changed();
    void gateRateChanged();
    void gateDepthChanged();
    void gateWidthChanged();
    void gateSmoothingChanged();

private:
    std::shared_ptr<NoiseFxDevice> m_device;
    std::vector<NoiseFxSweepController *> m_sweeps;
};

} // namespace noteahead

#endif // NOISE_FX_CONTROLLER_HPP
