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

#include "noise_fx_sweep_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/noise_fx_device.hpp"
#include "noise_fx_controller.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

NoiseFxSweepController::NoiseFxSweepController(size_t index, NoiseFxController & owner)
  : QObject { &owner }
  , m_index { index }
  , m_owner { owner }
{
}

int NoiseFxSweepController::index() const
{
    return static_cast<int>(m_index);
}

int NoiseFxSweepController::knob(const QString & key) const
{
    const auto device = m_owner.noiseFxDevice();
    return device ? static_cast<int>(std::round(device->sweepParameterValue(m_index, key) * Constants::uiInternalScaling())) : 0;
}

void NoiseFxSweepController::setKnob(const QString & key, int value)
{
    if (const auto device = m_owner.noiseFxDevice(); device) {
        device->setSweepParameterValue(m_index, key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int NoiseFxSweepController::choice(const QString & key) const
{
    const auto device = m_owner.noiseFxDevice();
    return device ? static_cast<int>(std::lround(device->sweepParameterValue(m_index, key))) : 0;
}

void NoiseFxSweepController::setChoice(const QString & key, int value)
{
    if (const auto device = m_owner.noiseFxDevice(); device) {
        device->setSweepParameterValue(m_index, key, static_cast<float>(value));
    }
}

int NoiseFxSweepController::target(size_t destination) const
{
    const auto device = m_owner.noiseFxDevice();
    return device ? static_cast<int>(device->sweepTarget(m_index, destination)) : 0;
}

void NoiseFxSweepController::setTarget(size_t destination, int value)
{
    if (const auto device = m_owner.noiseFxDevice(); device) {
        device->setSweepTarget(m_index, destination, static_cast<NoiseFxDevice::Destination>(value));
    }
}

int NoiseFxSweepController::depth(size_t destination) const
{
    const auto device = m_owner.noiseFxDevice();
    return device ? static_cast<int>(std::round(device->sweepDepth(m_index, destination) * Constants::uiInternalScaling())) : 0;
}

void NoiseFxSweepController::setDepth(size_t destination, int value)
{
    if (const auto device = m_owner.noiseFxDevice(); device) {
        device->setSweepDepth(m_index, destination, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int NoiseFxSweepController::start() const
{
    return knob(NahdXml::xmlKeyStart());
}

void NoiseFxSweepController::setStart(int value)
{
    setKnob(NahdXml::xmlKeyStart(), value);
}

int NoiseFxSweepController::end() const
{
    return knob(NahdXml::xmlKeyEnd());
}

void NoiseFxSweepController::setEnd(int value)
{
    setKnob(NahdXml::xmlKeyEnd(), value);
}

int NoiseFxSweepController::curve() const
{
    return knob(NahdXml::xmlKeyCurve());
}

void NoiseFxSweepController::setCurve(int value)
{
    setKnob(NahdXml::xmlKeyCurve(), value);
}

int NoiseFxSweepController::length() const
{
    return knob(NahdXml::xmlKeyLength());
}

void NoiseFxSweepController::setLength(int value)
{
    setKnob(NahdXml::xmlKeyLength(), value);
}

int NoiseFxSweepController::mode() const
{
    return choice(NahdXml::xmlKeyMode());
}

void NoiseFxSweepController::setMode(int value)
{
    setChoice(NahdXml::xmlKeyMode(), value);
}

bool NoiseFxSweepController::sync() const
{
    return choice(NahdXml::xmlKeySync()) != 0;
}

void NoiseFxSweepController::setSync(bool value)
{
    setChoice(NahdXml::xmlKeySync(), value ? 1 : 0);
}

int NoiseFxSweepController::syncLength() const
{
    return choice(NahdXml::xmlKeySyncLength());
}

void NoiseFxSweepController::setSyncLength(int value)
{
    setChoice(NahdXml::xmlKeySyncLength(), value);
}

int NoiseFxSweepController::target1() const
{
    return target(0);
}

void NoiseFxSweepController::setTarget1(int value)
{
    setTarget(0, value);
}

int NoiseFxSweepController::depth1() const
{
    return depth(0);
}

void NoiseFxSweepController::setDepth1(int value)
{
    setDepth(0, value);
}

int NoiseFxSweepController::target2() const
{
    return target(1);
}

void NoiseFxSweepController::setTarget2(int value)
{
    setTarget(1, value);
}

int NoiseFxSweepController::depth2() const
{
    return depth(1);
}

void NoiseFxSweepController::setDepth2(int value)
{
    setDepth(1, value);
}

} // namespace noteahead
