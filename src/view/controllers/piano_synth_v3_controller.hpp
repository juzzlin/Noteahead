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

#ifndef PIANO_SYNTH_V3_CONTROLLER_HPP
#define PIANO_SYNTH_V3_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class PianoSynthV3Device;

class PianoSynthV3Controller : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int inharmonicity READ inharmonicity WRITE setInharmonicity NOTIFY inharmonicityChanged)
    Q_PROPERTY(int hammerHardness READ hammerHardness WRITE setHammerHardness NOTIFY hammerHardnessChanged)
    Q_PROPERTY(int stringDetune READ stringDetune WRITE setStringDetune NOTIFY stringDetuneChanged)
    Q_PROPERTY(int stretch READ stretch WRITE setStretch NOTIFY stretchChanged)
    Q_PROPERTY(int richness READ richness WRITE setRichness NOTIFY richnessChanged)
    Q_PROPERTY(int doubleDecay READ doubleDecay WRITE setDoubleDecay NOTIFY doubleDecayChanged)
    Q_PROPERTY(int attack READ attack WRITE setAttack NOTIFY attackChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int releaseTime READ releaseTime WRITE setReleaseTime NOTIFY releaseTimeChanged)
    Q_PROPERTY(int stereoWidth READ stereoWidth WRITE setStereoWidth NOTIFY stereoWidthChanged)

public:
    explicit PianoSynthV3Controller(std::shared_ptr<PianoSynthV3Device> device, QObject * parent = nullptr);
    ~PianoSynthV3Controller() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int brightness() const;
    void setBrightness(int value);
    int decay() const;
    void setDecay(int value);
    int inharmonicity() const;
    void setInharmonicity(int value);
    int hammerHardness() const;
    void setHammerHardness(int value);
    int stringDetune() const;
    void setStringDetune(int value);
    int stretch() const;
    void setStretch(int value);
    int richness() const;
    void setRichness(int value);
    int doubleDecay() const;
    int attack() const;
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    void setDoubleDecay(int value);
    void setAttack(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    int releaseTime() const;
    void setReleaseTime(int value);
    int stereoWidth() const;
    void setStereoWidth(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void brightnessChanged();
    void decayChanged();
    void inharmonicityChanged();
    void hammerHardnessChanged();
    void stringDetuneChanged();
    void stretchChanged();
    void richnessChanged();
    void doubleDecayChanged();
    void attackChanged();
    void velocitySensitivityChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();
    void releaseTimeChanged();
    void stereoWidthChanged();

public:
    void setDevice(std::shared_ptr<PianoSynthV3Device> device);

private:
    std::shared_ptr<PianoSynthV3Device> m_device;
};

} // namespace noteahead

#endif // PIANO_SYNTH_V3_CONTROLLER_HPP
