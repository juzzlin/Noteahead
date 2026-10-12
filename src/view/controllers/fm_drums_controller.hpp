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

#ifndef FM_DRUMS_CONTROLLER_HPP
#define FM_DRUMS_CONTROLLER_HPP

#include "drum_pad_controller.hpp"

#include <QStringList>
#include <QVariantList>

#include <vector>

namespace noteahead {

class FmDrumOperatorController;

//! The FM Drums dialog's controller: the selected pad's FM voice on top of what every drum machine
//! has. Continuous controls are knob positions scaled by Constants::uiInternalScaling(); the
//! discrete ones -- algorithm, noise target, noise filter -- are the plain index.
class FmDrumsController : public DrumPadController
{
    Q_OBJECT

    //! The four operator panels, in order. Constant: the objects live as long as the controller and
    //! follow the selected pad themselves.
    Q_PROPERTY(QVariantList operators READ operators CONSTANT)
    Q_PROPERTY(QStringList algorithmNames READ algorithmNames CONSTANT)
    Q_PROPERTY(QStringList operatorWaveformNames READ operatorWaveformNames CONSTANT)
    Q_PROPERTY(QStringList noiseFilterModeNames READ noiseFilterModeNames CONSTANT)

    // One notification for the lot: a pad is thirty-odd knobs, and re-reading them all when any one
    // moves costs nothing worth a signal each
    Q_PROPERTY(int algorithm READ algorithm WRITE setAlgorithm NOTIFY padChanged)
    Q_PROPERTY(int pitch READ pitch WRITE setPitch NOTIFY padChanged)
    Q_PROPERTY(int feedback READ feedback WRITE setFeedback NOTIFY padChanged)
    Q_PROPERTY(int pitchDepth READ pitchDepth WRITE setPitchDepth NOTIFY padChanged)
    Q_PROPERTY(int pitchDecay READ pitchDecay WRITE setPitchDecay NOTIFY padChanged)

    Q_PROPERTY(int noiseLevel READ noiseLevel WRITE setNoiseLevel NOTIFY padChanged)
    Q_PROPERTY(int noiseModulation READ noiseModulation WRITE setNoiseModulation NOTIFY padChanged)
    Q_PROPERTY(int noiseTarget READ noiseTarget WRITE setNoiseTarget NOTIFY padChanged)
    Q_PROPERTY(int noiseAttack READ noiseAttack WRITE setNoiseAttack NOTIFY padChanged)
    Q_PROPERTY(int noiseDecay READ noiseDecay WRITE setNoiseDecay NOTIFY padChanged)
    Q_PROPERTY(int noiseCurve READ noiseCurve WRITE setNoiseCurve NOTIFY padChanged)
    Q_PROPERTY(int noiseFilterMode READ noiseFilterMode WRITE setNoiseFilterMode NOTIFY padChanged)
    Q_PROPERTY(int noiseCutoff READ noiseCutoff WRITE setNoiseCutoff NOTIFY padChanged)
    Q_PROPERTY(int noiseResonance READ noiseResonance WRITE setNoiseResonance NOTIFY padChanged)

    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY padChanged)
    Q_PROPERTY(int modVelocitySensitivity READ modVelocitySensitivity WRITE setModVelocitySensitivity NOTIFY padChanged)

public:
    explicit FmDrumsController(std::shared_ptr<DeviceService> deviceService, QObject * parent = nullptr);

    QVariantList operators() const;
    QStringList algorithmNames() const;
    QStringList operatorWaveformNames() const;
    QStringList noiseFilterModeNames() const;

    int algorithm() const;
    void setAlgorithm(int algorithm);
    int pitch() const;
    void setPitch(int value);
    int feedback() const;
    void setFeedback(int value);
    int pitchDepth() const;
    void setPitchDepth(int value);
    int pitchDecay() const;
    void setPitchDecay(int value);

    int noiseLevel() const;
    void setNoiseLevel(int value);
    int noiseModulation() const;
    void setNoiseModulation(int value);
    int noiseTarget() const;
    void setNoiseTarget(int value);
    int noiseAttack() const;
    void setNoiseAttack(int value);
    int noiseDecay() const;
    void setNoiseDecay(int value);
    int noiseCurve() const;
    void setNoiseCurve(int value);
    int noiseFilterMode() const;
    void setNoiseFilterMode(int value);
    int noiseCutoff() const;
    void setNoiseCutoff(int value);
    int noiseResonance() const;
    void setNoiseResonance(int value);

    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int modVelocitySensitivity() const;
    void setModVelocitySensitivity(int value);

    void requestSettings() override;

    //! The selected pad's continuous control @p key as a scaled knob position. For the operators.
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    //! The selected pad's discrete control @p key as its plain index.
    int choice(const QString & key) const;
    void setChoice(const QString & key, int value);

signals:
    void padChanged();

protected:
    bool acceptsDevice(const DeviceS & device) const override;

private:
    std::vector<FmDrumOperatorController *> m_operators;
};

} // namespace noteahead

#endif // FM_DRUMS_CONTROLLER_HPP
