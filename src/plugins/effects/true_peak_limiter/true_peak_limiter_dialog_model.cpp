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

#include "true_peak_limiter_dialog_model.hpp"

#include "true_peak_limiter.hpp"

namespace noteahead {

TruePeakLimiterDialogModel::TruePeakLimiterDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float TruePeakLimiterDialogModel::reductionDb() const
{
    if (const auto limiter { std::dynamic_pointer_cast<TruePeakLimiter>(effect()) }; limiter) {
        return limiter->reductionDb();
    }
    return 0.0f;
}

float TruePeakLimiterDialogModel::outputTruePeakDb() const
{
    if (const auto limiter { std::dynamic_pointer_cast<TruePeakLimiter>(effect()) }; limiter) {
        return limiter->outputTruePeakDb();
    }
    return -70.0f;
}

float TruePeakLimiterDialogModel::outputTruePeakHoldDb() const
{
    if (const auto limiter { std::dynamic_pointer_cast<TruePeakLimiter>(effect()) }; limiter) {
        return limiter->outputTruePeakHoldDb();
    }
    return -70.0f;
}

void TruePeakLimiterDialogModel::resetPeakHold()
{
    if (const auto limiter { std::dynamic_pointer_cast<TruePeakLimiter>(effect()) }; limiter) {
        limiter->resetPeakHold();
    }
}

} // namespace noteahead
