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

#ifndef KICK_808_CONTROLLER_HPP
#define KICK_808_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class Kick808Device;

class Kick808Controller : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int tune READ tune WRITE setTune NOTIFY tuneChanged)
    Q_PROPERTY(int tone READ tone WRITE setTone NOTIFY toneChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int pitchDepth READ pitchDepth WRITE setPitchDepth NOTIFY pitchDepthChanged)
    Q_PROPERTY(int pitchDecay READ pitchDecay WRITE setPitchDecay NOTIFY pitchDecayChanged)
    Q_PROPERTY(int drive READ drive WRITE setDrive NOTIFY driveChanged)
    Q_PROPERTY(int glide READ glide WRITE setGlide NOTIFY glideChanged)
    Q_PROPERTY(bool keyTrack READ keyTrack WRITE setKeyTrack NOTIFY keyTrackChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)

public:
    explicit Kick808Controller(std::shared_ptr<Kick808Device> device, QObject * parent = nullptr);
    ~Kick808Controller() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int tune() const;
    void setTune(int value);
    int tone() const;
    void setTone(int value);
    int decay() const;
    void setDecay(int value);
    int pitchDepth() const;
    void setPitchDepth(int value);
    int pitchDecay() const;
    void setPitchDecay(int value);
    int drive() const;
    void setDrive(int value);
    int glide() const;
    void setGlide(int value);
    bool keyTrack() const;
    void setKeyTrack(bool value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void tuneChanged();
    void toneChanged();
    void decayChanged();
    void pitchDepthChanged();
    void pitchDecayChanged();
    void driveChanged();
    void glideChanged();
    void keyTrackChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();

public:
    void setDevice(std::shared_ptr<Kick808Device> device);

private:
    std::shared_ptr<Kick808Device> m_device;
};

} // namespace noteahead

#endif // KICK_808_CONTROLLER_HPP
