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

#include "phase_scope_dialog_model.hpp"

#include "phase_scope.hpp"

#include "../../../common/constants.hpp"

namespace noteahead {

PhaseScopeDialogModel::PhaseScopeDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

int PhaseScopeDialogModel::delaySamples() const
{
    const auto effect = std::dynamic_pointer_cast<PhaseScope>(this->effect());
    if (!effect) {
        return 0;
    }
    const auto delay = effect->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString());
    return delay ? static_cast<int>(PhaseScope::delaySamples(delay->get().shownValue(), effect->sampleRate())) : 0;
}

void PhaseScopeDialogModel::setDelaySamples(int samples)
{
    const auto effect = std::dynamic_pointer_cast<PhaseScope>(this->effect());
    if (!effect || effect->sampleRate() <= 0.0) {
        return;
    }
    const auto maxSamples = static_cast<int>(PhaseScope::delaySamples(1.0f, effect->sampleRate()));
    const double ms = static_cast<double>(std::clamp(samples, 0, maxSamples)) * 1000.0 / effect->sampleRate();
    setParameterValue(Constants::NahdXml::xmlKeyDelayTime(), static_cast<float>(ms / PhaseScope::MaxDelayMs));
}

void PhaseScopeDialogModel::setActive(bool active)
{
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(this->effect()); effect) {
        effect->setAnalysisEnabled(active);
    }
}

QVariantMap PhaseScopeDialogModel::reading() const
{
    QVariantMap map;
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(this->effect()); effect) {
        const auto reading = effect->reading();
        map["hasReference"] = reading.hasReference;
        map["correlation"] = reading.correlation;
        map["referenceDb"] = reading.referenceDb;
        map["signalDb"] = reading.signalDb;
        map["sumGainDb"] = reading.sumGainDb;
    }
    return map;
}

QVariantMap PhaseScopeDialogModel::capture(int maxPoints) const
{
    QVariantMap map;
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(this->effect()); effect) {
        const auto capture = effect->capture();
        // Every n-th sample, the same n for both, so that the two stay aligned
        const auto decimate = [](const std::vector<float> & samples, size_t stride) {
            QVariantList list;
            for (size_t i = 0; i < samples.size(); i += stride) {
                list.append(samples[i]);
            }
            return list;
        };
        const auto length = std::max(capture.reference.size(), capture.signal.size());
        const auto stride = std::max<size_t>(1, maxPoints > 0 ? (length + static_cast<size_t>(maxPoints) - 1) / static_cast<size_t>(maxPoints) : 1);
        map["reference"] = decimate(capture.reference, stride);
        map["signal"] = decimate(capture.signal, stride);
    }
    return map;
}

} // namespace noteahead
