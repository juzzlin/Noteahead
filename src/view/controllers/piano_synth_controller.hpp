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

#ifndef PIANO_SYNTH_CONTROLLER_HPP
#define PIANO_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class PianoSynthDevice;

class PianoSynthController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int inharmonicity READ inharmonicity WRITE setInharmonicity NOTIFY inharmonicityChanged)
    Q_PROPERTY(int releaseTime READ releaseTime WRITE setReleaseTime NOTIFY releaseTimeChanged)
    Q_PROPERTY(int stereoWidth READ stereoWidth WRITE setStereoWidth NOTIFY stereoWidthChanged)
    Q_PROPERTY(int hammerHardness READ hammerHardness WRITE setHammerHardness NOTIFY hammerHardnessChanged)

public:
    explicit PianoSynthController(std::shared_ptr<PianoSynthDevice> device, QObject * parent = nullptr);
    ~PianoSynthController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int brightness() const;
    void setBrightness(int value);
    int decay() const;
    void setDecay(int value);
    int inharmonicity() const;
    void setInharmonicity(int value);
    int releaseTime() const;
    void setReleaseTime(int value);
    int stereoWidth() const;
    void setStereoWidth(int value);
    int hammerHardness() const;
    void setHammerHardness(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void brightnessChanged();
    void decayChanged();
    void inharmonicityChanged();
    void releaseTimeChanged();
    void stereoWidthChanged();
    void hammerHardnessChanged();

public:
    void setDevice(std::shared_ptr<PianoSynthDevice> device);

private:
    std::shared_ptr<PianoSynthDevice> m_device;
};

} // namespace noteahead

#endif // PIANO_SYNTH_CONTROLLER_HPP
