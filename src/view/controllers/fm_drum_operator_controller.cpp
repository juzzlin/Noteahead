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

#include "fm_drum_operator_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/dsp/drum/fm_drum_engine.hpp"
#include "fm_drums_controller.hpp"

#include <algorithm>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

FmDrumOperatorController::FmDrumOperatorController(size_t index, FmDrumsController & owner)
  : QObject { &owner }
  , m_index { index }
  , m_owner { owner }
{
}

int FmDrumOperatorController::index() const
{
    return static_cast<int>(m_index);
}

QString FmDrumOperatorController::title() const
{
    return tr("Operator") + " " + QString::number(m_index + 1);
}

int FmDrumOperatorController::waveform() const
{
    return m_owner.choice(NahdXml::xmlKeyOperatorWaveform(m_index));
}

void FmDrumOperatorController::setWaveform(int waveform)
{
    m_owner.setChoice(NahdXml::xmlKeyOperatorWaveform(m_index), waveform);
}

int FmDrumOperatorController::ratio() const
{
    return m_owner.knob(NahdXml::xmlKeyOperatorRatio(m_index));
}

void FmDrumOperatorController::setRatio(int ratio)
{
    m_owner.setKnob(NahdXml::xmlKeyOperatorRatio(m_index), ratio);
}

int FmDrumOperatorController::level() const
{
    return m_owner.knob(NahdXml::xmlKeyOperatorLevel(m_index));
}

void FmDrumOperatorController::setLevel(int level)
{
    m_owner.setKnob(NahdXml::xmlKeyOperatorLevel(m_index), level);
}

int FmDrumOperatorController::attack() const
{
    return m_owner.knob(NahdXml::xmlKeyOperatorAttack(m_index));
}

void FmDrumOperatorController::setAttack(int attack)
{
    m_owner.setKnob(NahdXml::xmlKeyOperatorAttack(m_index), attack);
}

int FmDrumOperatorController::decay() const
{
    return m_owner.knob(NahdXml::xmlKeyOperatorDecay(m_index));
}

void FmDrumOperatorController::setDecay(int decay)
{
    m_owner.setKnob(NahdXml::xmlKeyOperatorDecay(m_index), decay);
}

int FmDrumOperatorController::curve() const
{
    return m_owner.knob(NahdXml::xmlKeyOperatorCurve(m_index));
}

void FmDrumOperatorController::setCurve(int curve)
{
    m_owner.setKnob(NahdXml::xmlKeyOperatorCurve(m_index), curve);
}

bool FmDrumOperatorController::carrier() const
{
    const auto & algorithms = FmSynthDevice::algorithms();
    const auto algorithm = std::clamp(static_cast<size_t>(m_owner.algorithm()), size_t { 0 }, algorithms.size() - 1);
    return algorithms.at(algorithm).carriers & (1u << m_index);
}

QVariantList FmDrumOperatorController::modulatedBy() const
{
    const auto & algorithms = FmSynthDevice::algorithms();
    const auto algorithm = std::clamp(static_cast<size_t>(m_owner.algorithm()), size_t { 0 }, algorithms.size() - 1);
    QVariantList sources;
    for (size_t source = 0; source < FmDrumEngine::OperatorCount; source++) {
        if (algorithms.at(algorithm).modulators.at(m_index) & (1u << source)) {
            sources << static_cast<int>(source) + 1;
        }
    }
    return sources;
}

bool FmDrumOperatorController::feedbackOperator() const
{
    return m_index == FmDrumEngine::FeedbackOperator;
}

} // namespace noteahead
