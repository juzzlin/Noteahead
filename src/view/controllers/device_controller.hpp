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
#include <QStringList>
#include <QVariantList>
#include <memory>

namespace noteahead {

class Device;
class PresetService;

class DeviceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(int pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(uint32_t sampleRate READ sampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetNamesChanged)
    Q_PROPERTY(int currentPresetIndex READ currentPresetIndex WRITE setCurrentPresetIndex NOTIFY currentPresetIndexChanged)
    Q_PROPERTY(bool currentPresetIsUserPreset READ currentPresetIsUserPreset NOTIFY currentPresetIndexChanged)

public:
    using DeviceS = std::shared_ptr<Device>;
    using DeviceControllerS = std::shared_ptr<DeviceController>;
    using PresetServiceS = std::shared_ptr<PresetService>;

    explicit DeviceController(QObject * parent = nullptr);
    ~DeviceController() override = default;

    virtual DeviceS device() const = 0;
    virtual bool setDevice(DeviceS device) = 0;

    void setPresetService(PresetServiceS presetService);

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

    //! The device's built-in patches. A device without any leaves this alone and still gets the
    //! user's own presets, which is the whole reason the list lives here rather than in the two
    //! controllers that happen to ship factory patches today.
    virtual QStringList factoryPresetNames() const;
    virtual void loadFactoryPreset(int index);

    //! The factory patches followed by the user's own, numbered continuously. A user preset is
    //! marked, so that the two are told apart without being separated.
    QStringList presetNames() const;
    int currentPresetIndex() const;
    void setCurrentPresetIndex(int index);
    bool currentPresetIsUserPreset() const;
    //! The selected user preset's name without the numbering the dropdown adds, or empty when the
    //! selected preset is a factory one. What a confirmation has to quote back at the user.
    Q_INVOKABLE QString currentUserPresetName() const;

    //! Loads the preset at @p index of presetNames(), factory or user alike.
    Q_INVOKABLE void loadPreset(int index);

    //! Whether saving under @p presetName would replace a preset the user already has.
    Q_INVOKABLE bool userPresetExists(const QString & presetName) const;
    //! Stores the device's current parameters under @p presetName and shows the result as selected.
    Q_INVOKABLE bool saveUserPreset(const QString & presetName);
    //! Deletes the selected preset. A no-op unless the selected one is the user's own.
    Q_INVOKABLE bool deleteCurrentUserPreset();

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
    void presetNamesChanged();
    void currentPresetIndexChanged();

protected:
    void connectDeviceSignals();
    //! Re-applies the desired scope-active state to the current device. Concrete controllers must
    //! call this after swapping the underlying device so the scope always follows the shown device
    //! and never keeps capturing on a device that is no longer displayed.
    void applyScopeActive();

private:
    //! The user's presets for the device's type, or an empty list when no store is set.
    QStringList userPresetNames() const;
    QString deviceTypeId() const;

    PresetServiceS m_presetService;
    int m_currentPresetIndex = 0;
    bool m_scopeActive = false;
    std::weak_ptr<Device> m_scopeDevice;
};

} // namespace noteahead

#endif // DEVICE_CONTROLLER_HPP
