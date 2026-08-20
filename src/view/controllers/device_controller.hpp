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

#ifndef DEVICE_CONTROLLER_HPP
#define DEVICE_CONTROLLER_HPP

#include <QObject>
#include <QVariantList>
#include <memory>

namespace noteahead {

class Device;

class DeviceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(int pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(uint32_t sampleRate READ sampleRate NOTIFY sampleRateChanged)

public:
    using DeviceS = std::shared_ptr<Device>;
    using DeviceControllerS = std::shared_ptr<DeviceController>;

    explicit DeviceController(QObject * parent = nullptr);
    ~DeviceController() override = default;

    virtual DeviceS device() const = 0;
    virtual bool setDevice(DeviceS device) = 0;

    int volume() const;
    void setVolume(int value);

    int gain() const;
    void setGain(int value);

    int pan() const;
    void setPan(int value);

    uint32_t sampleRate() const;

    Q_INVOKABLE float cutoffToHz(float cutoff) const;
    Q_INVOKABLE QString deviceName() const;

    //! Enable/disable the device's oscilloscope capture. Keep it enabled only while a scope is shown.
    Q_INVOKABLE void setScopeActive(bool active);
    //! Returns [leftSamples, rightSamples] as nested lists of floats in output amplitude units,
    //! decimated to at most maxPoints per channel. Empty when no device is set.
    //! \param cycles When positive, the scope returns that many periods of the pitch it hears
    //! rather than a fixed window, so the trace stands still as notes change.
    Q_INVOKABLE QVariantList scopeSamples(int maxPoints, int cycles = 0) const;
    Q_INVOKABLE int scopeSampleRate() const;

    //! Pitch the scope's last cycle-locked read found, in Hz, or 0 when there was none to find.
    Q_INVOKABLE double scopeFrequency() const;

    Q_INVOKABLE virtual void reset();
    Q_INVOKABLE virtual void requestSettings() = 0;
    Q_INVOKABLE virtual void accept();
    Q_INVOKABLE virtual void reject();

    Q_INVOKABLE virtual void playNote(int note, double velocity = 1.0);
    Q_INVOKABLE virtual void stopNote(int note);

signals:
    void volumeChanged();
    void gainChanged();
    void panChanged();
    void sampleRateChanged();

protected:
    void connectDeviceSignals();
    //! Re-applies the desired scope-active state to the current device. Concrete controllers must
    //! call this after swapping the underlying device so the scope always follows the shown device
    //! and never keeps capturing on a device that is no longer displayed.
    void applyScopeActive();

private:
    bool m_scopeActive = false;
    std::weak_ptr<Device> m_scopeDevice;
};

} // namespace noteahead

#endif // DEVICE_CONTROLLER_HPP
