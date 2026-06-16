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

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/device.hpp"

#include <QDebug>
#include <cmath>

namespace noteahead {

DeviceController::DeviceController(QObject * parent)
  : QObject { parent }
{
}

int DeviceController::volume() const
{
    auto dev = device();
    return dev ? static_cast<int>(std::round(dev->volume() * Constants::uiInternalScaling())) : 1000;
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

void DeviceController::reset()
{
    if (auto dev = device(); dev) {
        dev->reset();
    }
    requestSettings();
}

void DeviceController::accept()
{
}

void DeviceController::reject()
{
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
    }
}

} // namespace noteahead
