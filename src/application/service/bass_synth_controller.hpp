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

#ifndef BASS_SYNTH_CONTROLLER_HPP
#define BASS_SYNTH_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class BassSynthDevice;

class BassSynthController : public DeviceController
{
    Q_OBJECT

    // Oscillator
    Q_PROPERTY(int waveform READ waveform WRITE setWaveform NOTIFY waveformChanged)
    Q_PROPERTY(int tuning READ tuning WRITE setTuning NOTIFY tuningChanged)
    Q_PROPERTY(int subLevel READ subLevel WRITE setSubLevel NOTIFY subLevelChanged)
    Q_PROPERTY(int subOctave READ subOctave WRITE setSubOctave NOTIFY subOctaveChanged)

    // Filter
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int envMod READ envMod WRITE setEnvMod NOTIFY envModChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)

    // Global / Modifiers
    Q_PROPERTY(int accent READ accent WRITE setAccent NOTIFY accentChanged)
    Q_PROPERTY(int slide READ slide WRITE setSlide NOTIFY slideChanged)

    // Distortion
    Q_PROPERTY(int distDrive READ distDrive WRITE setDistDrive NOTIFY distDriveChanged)
    Q_PROPERTY(int distTone READ distTone WRITE setDistTone NOTIFY distToneChanged)
    Q_PROPERTY(int distLevel READ distLevel WRITE setDistLevel NOTIFY distLevelChanged)

public:
    explicit BassSynthController(std::shared_ptr<BassSynthDevice> device, QObject * parent = nullptr);
    ~BassSynthController() override;

    std::shared_ptr<Device> device() const override;
    std::shared_ptr<BassSynthDevice> bassSynthDevice() const;

    // Accessors
    int waveform() const;
    void setWaveform(int wave);
    int tuning() const;
    void setTuning(int t);
    int subLevel() const;
    void setSubLevel(int level);
    int subOctave() const;
    void setSubOctave(int oct);

    int lpfCutoff() const;
    void setLpfCutoff(int c);
    int lpfResonance() const;
    void setLpfResonance(int r);
    int hpfCutoff() const;
    void setHpfCutoff(int c);
    int envMod() const;
    void setEnvMod(int m);
    int decay() const;
    void setDecay(int d);

    int accent() const;
    void setAccent(int a);
    int slide() const;
    void setSlide(int s);

    int distDrive() const;
    void setDistDrive(int d);
    int distTone() const;
    void setDistTone(int t);
    int distLevel() const;
    void setDistLevel(int l);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void waveformChanged();
    void tuningChanged();
    void subLevelChanged();
    void subOctaveChanged();
    void lpfCutoffChanged();
    void lpfResonanceChanged();
    void hpfCutoffChanged();
    void envModChanged();
    void decayChanged();
    void accentChanged();
    void slideChanged();
    void distDriveChanged();
    void distToneChanged();
    void distLevelChanged();

public:
    void setDevice(std::shared_ptr<BassSynthDevice> device);

private:
    std::shared_ptr<BassSynthDevice> m_device;
};

} // namespace noteahead

#endif // BASS_SYNTH_CONTROLLER_HPP
