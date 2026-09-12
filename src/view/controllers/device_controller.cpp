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

#include "device_controller.hpp"

#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/device.hpp"

#include <QDebug>
#include <algorithm>
#include <cmath>

namespace noteahead {

DeviceController::DeviceController(QObject * parent)
  : QObject { parent }
{
}

void DeviceController::setPresetService(PresetServiceS presetService)
{
    m_presetService = std::move(presetService);
    emit presetNamesChanged();
}

QStringList DeviceController::factoryPresetNames() const
{
    return {};
}

void DeviceController::loadFactoryPreset(int)
{
}

QString DeviceController::deviceTypeId() const
{
    const auto dev = device();
    return dev ? QString::fromStdString(dev->typeId()) : QString {};
}

QStringList DeviceController::userPresetNames() const
{
    if (const auto typeId = deviceTypeId(); m_presetService && !typeId.isEmpty()) {
        return m_presetService->userPresetNames(typeId);
    }
    return {};
}

QStringList DeviceController::presetNames() const
{
    QStringList names;
    const auto append = [&names](const QString & name, bool isUserPreset) {
        // The number is the position in this very list, so that what the dropdown shows and what
        // loadPreset() is given are the same thing however many presets the user has added.
        names.append(QString { "%1: %2%3" }
                       .arg(names.size(), 3, 10, QChar { '0' })
                       .arg(name)
                       .arg(isUserPreset ? Constants::userPresetMarker() : QString {}));
    };
    for (const auto & name : factoryPresetNames()) {
        append(name, false);
    }
    for (const auto & name : userPresetNames()) {
        append(name, true);
    }
    return names;
}

int DeviceController::currentPresetIndex() const
{
    return m_currentPresetIndex;
}

void DeviceController::setCurrentPresetIndex(int index)
{
    if (m_currentPresetIndex != index) {
        m_currentPresetIndex = index;
        emit currentPresetIndexChanged();
    }
}

bool DeviceController::currentPresetIsUserPreset() const
{
    const auto userIndex = m_currentPresetIndex - static_cast<int>(factoryPresetNames().size());
    return userIndex >= 0 && userIndex < static_cast<int>(userPresetNames().size());
}

QString DeviceController::currentUserPresetName() const
{
    if (!currentPresetIsUserPreset()) {
        return {};
    }
    return userPresetNames().at(m_currentPresetIndex - static_cast<int>(factoryPresetNames().size()));
}

void DeviceController::loadPreset(int index)
{
    // The controller owns which preset is showing, so that the dialog only has to ask for one to be
    // loaded. Left to the caller, the combo box reads back its old value and snaps back.
    setCurrentPresetIndex(index);

    const auto factoryCount = static_cast<int>(factoryPresetNames().size());
    if (index < factoryCount) {
        loadFactoryPreset(index);
        return;
    }

    const auto names = userPresetNames();
    if (const auto userIndex = index - factoryCount; userIndex < static_cast<int>(names.size())) {
        if (const auto dev = device(); dev && m_presetService) {
            m_presetService->applyUserPreset(deviceTypeId(), names.at(userIndex), *dev);
            requestSettings();
        }
    }
}

bool DeviceController::userPresetExists(const QString & presetName) const
{
    const auto typeId = deviceTypeId();
    return m_presetService && !typeId.isEmpty() && m_presetService->userPresetExists(typeId, presetName);
}

bool DeviceController::saveUserPreset(const QString & presetName)
{
    const auto dev = device();
    const auto typeId = deviceTypeId();
    if (!dev || !m_presetService || typeId.isEmpty()) {
        return false;
    }

    if (!m_presetService->saveUserPreset(typeId, presetName, *dev)) {
        return false;
    }

    emit presetNamesChanged();

    // Show what was just saved as the selected preset: the user named this patch, so the dropdown
    // has to agree that this is the patch they are on.
    if (const auto index = userPresetNames().indexOf(presetName); index >= 0) {
        setCurrentPresetIndex(static_cast<int>(factoryPresetNames().size()) + index);
    }

    return true;
}

bool DeviceController::deleteCurrentUserPreset()
{
    if (!currentPresetIsUserPreset() || !m_presetService) {
        return false;
    }

    const auto factoryCount = static_cast<int>(factoryPresetNames().size());
    if (!m_presetService->deleteUserPreset(deviceTypeId(), userPresetNames().at(m_currentPresetIndex - factoryCount))) {
        return false;
    }

    emit presetNamesChanged();

    // The list just got shorter under the selection. Landing on the preset that took the deleted
    // one's place keeps the dropdown from pointing past the end of itself. Emitted even when the
    // index itself does not move, because whether it names a user preset just changed.
    const auto presetCount = factoryCount + static_cast<int>(userPresetNames().size());
    m_currentPresetIndex = std::clamp(m_currentPresetIndex, 0, std::max(0, presetCount - 1));
    emit currentPresetIndexChanged();

    return true;
}

int DeviceController::volume() const
{
    auto dev = device();
    // Unity, which is also the sane answer for an absent device
    return dev ? static_cast<int>(std::round(dev->volume() * Constants::uiInternalScaling())) : static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling()));
}

void DeviceController::setVolume(int value)
{
    if (auto dev = device(); dev) {
        dev->setVolume(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DeviceController::gain() const
{
    auto dev = device();
    return dev ? static_cast<int>(std::round(dev->gain() * Constants::uiInternalScaling())) : 500;
}

void DeviceController::setGain(int value)
{
    if (auto dev = device(); dev) {
        dev->setGain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DeviceController::pan() const
{
    auto dev = device();
    return dev ? static_cast<int>(std::round(dev->pan() * Constants::uiInternalScaling())) : 500;
}

void DeviceController::setPan(int value)
{
    if (auto dev = device(); dev) {
        dev->setPan(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

uint32_t DeviceController::sampleRate() const
{
    auto dev = device();
    return dev ? dev->sampleRate() : static_cast<uint32_t>(Constants::defaultSampleRate());
}

float DeviceController::cutoffToHz(float cutoff) const
{
    return Utils::Dsp::cutoffToHz(cutoff / Constants::uiInternalScaling(), static_cast<float>(sampleRate()));
}

QString DeviceController::deviceName() const
{
    if (const auto dev = device(); dev) {
        return QString::fromStdString(dev->name());
    }
    return {};
}

void DeviceController::setScopeActive(bool active)
{
    m_scopeActive = active;
    applyScopeActive();
}

void DeviceController::applyScopeActive()
{
    const auto current = device();
    // Disable capture on any device we previously enabled so switching instances (or repeated
    // toggles) can never leave a stale scope running in the audio thread while nothing is shown.
    if (const auto previous = m_scopeDevice.lock(); previous && previous != current) {
        previous->scope().setActive(false);
    }
    if (current) {
        current->scope().setActive(m_scopeActive);
    }
    m_scopeDevice = m_scopeActive ? current : DeviceS {};
}

QVariantList DeviceController::scopeSamples(int maxPoints, int cycles) const
{
    QVariantList result;
    if (const auto dev = device(); dev) {
        const auto snapshot = dev->scope().snapshot(static_cast<size_t>(std::max(0, maxPoints)), cycles);
        const auto toList = [](const std::vector<float> & samples) {
            QVariantList list;
            list.reserve(static_cast<int>(samples.size()));
            for (const auto sample : samples) {
                list.append(sample);
            }
            return list;
        };
        result.append(QVariant { toList(snapshot.left) });
        result.append(QVariant { toList(snapshot.right) });
    }
    return result;
}

double DeviceController::scopeFrequency() const
{
    if (const auto dev = device(); dev) {
        return dev->scope().lastDetectedFrequency();
    }
    return 0.0;
}

int DeviceController::scopeSampleRate() const
{
    if (const auto dev = device(); dev) {
        return static_cast<int>(dev->scope().sampleRate());
    }
    return 0;
}

void DeviceController::reset()
{
    if (auto dev = device(); dev) {
        dev->reset();
    }
    requestSettings();
}

void DeviceController::accept()
{
    // Nothing to commit: the edits have been live in the device all along. Only Cancel does work.
}

void DeviceController::reject()
{
    if (auto dev = device(); dev) {
        dev->restoreState();
    }
    requestSettings();
}

void DeviceController::playNote(int note, double velocity)
{
    if (auto dev = device(); dev) {
        dev->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
    }
}

void DeviceController::stopNote(int note)
{
    if (auto dev = device(); dev) {
        dev->processMidiNoteOff(static_cast<uint8_t>(note));
    }
}

void DeviceController::connectDeviceSignals()
{
    if (auto dev = device(); dev) {
        connect(dev.get(), &Device::sampleRateChanged, this, &DeviceController::requestSettings, Qt::UniqueConnection);
        connect(dev.get(), &Device::dataChanged, this, &DeviceController::requestSettings, Qt::UniqueConnection);
        // The open dialog still follows incoming MIDI CC, but nothing else in the application does
        connect(dev.get(), &Device::parametersChanged, this, &DeviceController::requestSettings, Qt::UniqueConnection);
    }
}

} // namespace noteahead
