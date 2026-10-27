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

#ifndef SNARE_SYNTH_CONTROLLER_HPP
#define SNARE_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class SnareSynthDevice;

class SnareSynthController : public DeviceController
{
    Q_OBJECT

    // Head / Membrane parameters
    Q_PROPERTY(int tune READ tune WRITE setTune NOTIFY tuneChanged)
    Q_PROPERTY(int tone READ tone WRITE setTone NOTIFY toneChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int headTension READ headTension WRITE setHeadTension NOTIFY headTensionChanged)
    Q_PROPERTY(int pitchDepth READ pitchDepth WRITE setPitchDepth NOTIFY pitchDepthChanged)
    Q_PROPERTY(int pitchDecay READ pitchDecay WRITE setPitchDecay NOTIFY pitchDecayChanged)

    // Snare wire parameters
    Q_PROPERTY(int snappy READ snappy WRITE setSnappy NOTIFY snappyChanged)
    Q_PROPERTY(int snareDecay READ snareDecay WRITE setSnareDecay NOTIFY snareDecayChanged)
    Q_PROPERTY(int snareTension READ snareTension WRITE setSnareTension NOTIFY snareTensionChanged)
    Q_PROPERTY(int snareBuzz READ snareBuzz WRITE setSnareBuzz NOTIFY snareBuzzChanged)

    // Stick strike & body parameters
    Q_PROPERTY(int click READ click WRITE setClick NOTIFY clickChanged)
    Q_PROPERTY(int hardness READ hardness WRITE setHardness NOTIFY hardnessChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int body READ body WRITE setBody NOTIFY bodyChanged)
    Q_PROPERTY(int drive READ drive WRITE setDrive NOTIFY driveChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)

    // Playback parameters
    Q_PROPERTY(bool chromatic READ chromatic WRITE setChromatic NOTIFY chromaticChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)

    // Output filters
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)

public:
    explicit SnareSynthController(std::shared_ptr<SnareSynthDevice> device, QObject * parent = nullptr);
    ~SnareSynthController() override;

    DeviceController::DeviceS device() const override;
    bool setDevice(DeviceController::DeviceS device) override;
    void setDevice(std::shared_ptr<SnareSynthDevice> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    // Head / Membrane
    int tune() const;
    void setTune(int value);
    int tone() const;
    void setTone(int value);
    int decay() const;
    void setDecay(int value);
    int headTension() const;
    void setHeadTension(int value);
    int pitchDepth() const;
    void setPitchDepth(int value);
    int pitchDecay() const;
    void setPitchDecay(int value);

    // Snare wires
    int snappy() const;
    void setSnappy(int value);
    int snareDecay() const;
    void setSnareDecay(int value);
    int snareTension() const;
    void setSnareTension(int value);
    int snareBuzz() const;
    void setSnareBuzz(int value);

    // Stick strike & body
    int click() const;
    void setClick(int value);
    int hardness() const;
    void setHardness(int value);
    int position() const;
    void setPosition(int value);
    int body() const;
    void setBody(int value);
    int drive() const;
    void setDrive(int value);
    int panSpread() const;
    void setPanSpread(int value);

    // Playback
    bool chromatic() const;
    void setChromatic(bool value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);

    // Output filters
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
    void headTensionChanged();
    void pitchDepthChanged();
    void pitchDecayChanged();

    void snappyChanged();
    void snareDecayChanged();
    void snareTensionChanged();
    void snareBuzzChanged();

    void clickChanged();
    void hardnessChanged();
    void positionChanged();
    void bodyChanged();
    void driveChanged();
    void panSpreadChanged();

    void chromaticChanged();
    void velocitySensitivityChanged();

    void lpfCutoffChanged();
    void hpfCutoffChanged();

private:
    void connectDeviceSignals();

    std::shared_ptr<SnareSynthDevice> m_device;
};

} // namespace noteahead

#endif // SNARE_SYNTH_CONTROLLER_HPP
