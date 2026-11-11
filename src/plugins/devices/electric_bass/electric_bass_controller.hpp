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

#ifndef ELECTRIC_BASS_CONTROLLER_HPP
#define ELECTRIC_BASS_CONTROLLER_HPP

#include "../../../view/controllers/device_controller.hpp"

#include <QTimer>
#include <QVariantList>

#include <memory>

namespace noteahead {

class ElectricBassDevice;

//! The Electric Bass as its dialog sees it: one property per knob, scaled by
//! Constants::uiInternalScaling(), and one note rendered for the waveform view.
class ElectricBassController : public DeviceController
{
    Q_OBJECT

    // Every knob changes with the others on a preset or a reload, so they share one signal
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY knobsChanged)
    Q_PROPERTY(int finger READ finger WRITE setFinger NOTIFY knobsChanged)
    Q_PROPERTY(int sustain READ sustain WRITE setSustain NOTIFY knobsChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY knobsChanged)
    Q_PROPERTY(int release READ release WRITE setRelease NOTIFY knobsChanged)
    Q_PROPERTY(int neck READ neck WRITE setNeck NOTIFY knobsChanged)
    Q_PROPERTY(int bridge READ bridge WRITE setBridge NOTIFY knobsChanged)
    Q_PROPERTY(int tone READ tone WRITE setTone NOTIFY knobsChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY knobsChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY knobsChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY knobsChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY knobsChanged)

    // One note, rendered for the waveform view
    Q_PROPERTY(QVariantList waveformData READ waveformData NOTIFY waveformChanged)
    Q_PROPERTY(double waveformDuration READ waveformDuration NOTIFY waveformChanged)

public:
    explicit ElectricBassController(std::shared_ptr<ElectricBassDevice> device, QObject * parent = nullptr);
    ~ElectricBassController() override;

    DeviceController::DeviceS device() const override;
    bool setDevice(DeviceController::DeviceS device) override;
    void setDevice(std::shared_ptr<ElectricBassDevice> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int position() const;
    void setPosition(int value);
    int finger() const;
    void setFinger(int value);
    int sustain() const;
    void setSustain(int value);
    int brightness() const;
    void setBrightness(int value);
    int release() const;
    void setRelease(int value);
    int neck() const;
    void setNeck(int value);
    int bridge() const;
    void setBridge(int value);
    int tone() const;
    void setTone(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int pitchBendRange() const;
    void setPitchBendRange(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);

    QVariantList waveformData() const;
    double waveformDuration() const;
    //! How many points the waveform view has room for, and whether it is on screen: the note is only
    //! rendered while someone can see it.
    Q_INVOKABLE void setWaveformRequest(int peakCount, bool visible);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void knobsChanged();
    void waveformChanged();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    //! Waits for a drag to settle before rendering: a note is seconds of audio
    void scheduleWaveform(int delayMs = WaveformDebounceMs);
    void renderWaveform();

    static constexpr int WaveformDebounceMs { 150 };

    QTimer m_waveformTimer;
    int m_waveformPeakCount { 0 };
    bool m_waveformVisible { false };
    QVariantList m_waveformData;
    double m_waveformDuration { 0.0 };

    std::shared_ptr<ElectricBassDevice> m_device;
};

} // namespace noteahead

#endif // ELECTRIC_BASS_CONTROLLER_HPP
