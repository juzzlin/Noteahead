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

#include "fm_operator_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/fm_synth_device.hpp"

#include <QVariantList>

#include <cmath>

namespace noteahead {

namespace {

int toUi(float value)
{
    return static_cast<int>(std::round(value * Constants::uiInternalScaling()));
}

float fromUi(int value)
{
    return static_cast<float>(value) / Constants::uiInternalScaling();
}

} // namespace

FmOperatorController::FmOperatorController(size_t index, QObject * parent)
  : QObject { parent }
  , m_index { index }
{
}

void FmOperatorController::setSynth(std::shared_ptr<FmSynthDevice> synth)
{
    m_synth = std::move(synth);
    refresh();
}

void FmOperatorController::refresh()
{
    emit changed();
    emit routingChanged();
}

void FmOperatorController::refreshRouting()
{
    emit routingChanged();
}

int FmOperatorController::index() const
{
    return static_cast<int>(m_index);
}

QString FmOperatorController::title() const
{
    return tr("Operator") + " " + QString::number(m_index + 1);
}

int FmOperatorController::waveform() const
{
    return m_synth ? static_cast<int>(m_synth->operatorWaveform(m_index)) : 0;
}

void FmOperatorController::setWaveform(int waveform)
{
    if (m_synth) {
        m_synth->setOperatorWaveform(m_index, static_cast<FmOperator::Waveform>(waveform));
    }
}

int FmOperatorController::ratio() const
{
    return m_synth ? m_synth->operatorRatio(m_index) : 1;
}

void FmOperatorController::setRatio(int ratio)
{
    if (m_synth) {
        m_synth->setOperatorRatio(m_index, ratio);
    }
}

QString FmOperatorController::ratioText() const
{
    const double value = FmSynthDevice::ratioForSetting(ratio());
    // The only fractional entry on the grid is the half, so everything else reads as a whole number
    // rather than as a decimal with nothing after the point.
    return value < 1.0 ? QString::number(value, 'f', 1) : QString::number(static_cast<int>(value));
}

int FmOperatorController::detune() const
{
    return m_synth ? toUi(m_synth->operatorDetune(m_index)) : 0;
}

void FmOperatorController::setDetune(int detune)
{
    if (m_synth) {
        m_synth->setOperatorDetune(m_index, fromUi(detune));
    }
}

int FmOperatorController::level() const
{
    return m_synth ? toUi(m_synth->operatorLevel(m_index)) : 0;
}

void FmOperatorController::setLevel(int level)
{
    if (m_synth) {
        m_synth->setOperatorLevel(m_index, fromUi(level));
    }
}

int FmOperatorController::velocitySensitivity() const
{
    return m_synth ? toUi(m_synth->operatorVelocitySensitivity(m_index)) : 0;
}

void FmOperatorController::setVelocitySensitivity(int sensitivity)
{
    if (m_synth) {
        m_synth->setOperatorVelocitySensitivity(m_index, fromUi(sensitivity));
    }
}

int FmOperatorController::keyScale() const
{
    return m_synth ? toUi(m_synth->operatorKeyScale(m_index)) : 0;
}

void FmOperatorController::setKeyScale(int keyScale)
{
    if (m_synth) {
        m_synth->setOperatorKeyScale(m_index, fromUi(keyScale));
    }
}

int FmOperatorController::attack() const
{
    return m_synth ? toUi(m_synth->operatorAttack(m_index)) : 0;
}

void FmOperatorController::setAttack(int a)
{
    if (m_synth) {
        m_synth->setOperatorAttack(m_index, fromUi(a));
    }
}

int FmOperatorController::decay() const
{
    return m_synth ? toUi(m_synth->operatorDecay(m_index)) : 0;
}

void FmOperatorController::setDecay(int d)
{
    if (m_synth) {
        m_synth->setOperatorDecay(m_index, fromUi(d));
    }
}

int FmOperatorController::sustain() const
{
    return m_synth ? toUi(m_synth->operatorSustain(m_index)) : 0;
}

void FmOperatorController::setSustain(int s)
{
    if (m_synth) {
        m_synth->setOperatorSustain(m_index, fromUi(s));
    }
}

bool FmOperatorController::carrier() const
{
    if (!m_synth) {
        return false;
    }
    const auto & algorithm = FmSynthDevice::algorithms().at(static_cast<size_t>(m_synth->algorithm()));
    return algorithm.carriers & (1u << m_index);
}

QVariantList FmOperatorController::modulatedBy() const
{
    QVariantList sources;
    if (!m_synth) {
        return sources;
    }
    const auto & algorithm = FmSynthDevice::algorithms().at(static_cast<size_t>(m_synth->algorithm()));
    for (size_t source = 0; source < FmSynthDevice::OperatorCount; source++) {
        if (algorithm.modulators.at(m_index) & (1u << source)) {
            sources << static_cast<int>(source) + 1;
        }
    }
    return sources;
}

bool FmOperatorController::feedbackOperator() const
{
    return m_index == FmSynthDevice::FeedbackOperator;
}

} // namespace noteahead
