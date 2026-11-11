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

#ifndef PIANO_SYNTH_ACOUSTIC_CONTROLLER_HPP
#define PIANO_SYNTH_ACOUSTIC_CONTROLLER_HPP

#include "../../../view/controllers/device_controller.hpp"

#include <memory>

namespace noteahead {

class PianoSynthAcousticDevice;

//! Piano Synth Acoustic as its dialog sees it: one property per knob, scaled by
//! Constants::uiInternalScaling().
class PianoSynthAcousticController : public DeviceController
{
    Q_OBJECT

    // Every knob changes with the others on a preset or a reload, so they share one signal
    Q_PROPERTY(int hardness READ hardness WRITE setHardness NOTIFY knobsChanged)
    Q_PROPERTY(int knock READ knock WRITE setKnock NOTIFY knobsChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY knobsChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY knobsChanged)
    Q_PROPERTY(int inharmonicity READ inharmonicity WRITE setInharmonicity NOTIFY knobsChanged)
    Q_PROPERTY(int detune READ detune WRITE setDetune NOTIFY knobsChanged)
    Q_PROPERTY(int release READ release WRITE setRelease NOTIFY knobsChanged)
    Q_PROPERTY(int resonance READ resonance WRITE setResonance NOTIFY knobsChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY knobsChanged)
    Q_PROPERTY(int stretch READ stretch WRITE setStretch NOTIFY knobsChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY knobsChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY knobsChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY knobsChanged)

public:
    explicit PianoSynthAcousticController(std::shared_ptr<PianoSynthAcousticDevice> device, QObject * parent = nullptr);
    ~PianoSynthAcousticController() override;

    DeviceController::DeviceS device() const override;
    bool setDevice(DeviceController::DeviceS device) override;
    void setDevice(std::shared_ptr<PianoSynthAcousticDevice> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int hardness() const;
    void setHardness(int value);
    int knock() const;
    void setKnock(int value);
    int decay() const;
    void setDecay(int value);
    int brightness() const;
    void setBrightness(int value);
    int inharmonicity() const;
    void setInharmonicity(int value);
    int detune() const;
    void setDetune(int value);
    int release() const;
    void setRelease(int value);
    int resonance() const;
    void setResonance(int value);
    int panSpread() const;
    void setPanSpread(int value);
    int stretch() const;
    void setStretch(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void knobsChanged();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);

    std::shared_ptr<PianoSynthAcousticDevice> m_device;
};

} // namespace noteahead

#endif // PIANO_SYNTH_ACOUSTIC_CONTROLLER_HPP
