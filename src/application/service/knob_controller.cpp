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

#include "knob_controller.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <cmath>
#include <vector>

namespace noteahead {

static const std::vector<double> syncDivisions = { 1.0, 0.75, 0.5, 0.375, 1.0 / 3.0, 0.25, 0.1875, 1.0 / 6.0, 0.125, 0.09375, 1.0 / 12.0, 0.0625, 0.046875, 1.0 / 24.0, 0.03125, 0.015625 };
static const std::vector<QString> syncLabels = { "1/1", "3/4", "1/2", "3/8", "1/3", "1/4", "3/16", "1/6", "1/8", "3/32", "1/12", "1/16", "3/64", "1/24", "1/32", "1/64" };

KnobController::KnobController(QObject * parent)
    : QObject(parent)
{
}

double KnobController::map(double value, const QString & type, double min, double max) const
{
    if (type == "exponential") return ParameterMapper::mapExponential(value, min, max);
    if (type == "cubic") return ParameterMapper::mapCubic(value, min, max);
    if (type == "cubicCentered" || type == "pan") return ParameterMapper::mapCubicCentered(value * 2.0 - 1.0, min, max);
    if (type == "intensity") return mapIntensity(value * 2.0 - 1.0, min, max);
    if (type == "logFrequency") return ParameterMapper::mapLogFrequency(value, min, max);
    return min + (value * (max - min)); // linear
}

double KnobController::unmap(double mappedValue, const QString & type, double min, double max) const
{
    if (type == "exponential") return ParameterMapper::unmapExponential(mappedValue, min, max);
    if (type == "cubic") return ParameterMapper::unmapCubic(mappedValue, min, max);
    if (type == "cubicCentered" || type == "pan") return (ParameterMapper::unmapCubicCentered(mappedValue, min, max) + 1.0) / 2.0;
    if (type == "intensity") return (unmapIntensity(mappedValue, min, max) + 1.0) / 2.0;
    if (type == "logFrequency") return ParameterMapper::unmapLogFrequency(mappedValue, min, max);
    return (max != min) ? (mappedValue - min) / (max - min) : 0.0;
}

QString KnobController::format(double mappedValue, const QString & type, const QString & suffix, double min, double max) const
{
    if (type == "pan") {
        return panToString(mappedValue, min, max);
    }
    if (type == "intensity" || type == "cubicCentered") {
        return intensityToString(mappedValue, min, max);
    }
    if (type == "logFrequency" || type == "frequency") {
        const double linearValue = unmap(mappedValue, type, min, max) * Constants::uiInternalScaling();
        const QString freqStr = frequencyToString(linearValue, mappedValue, false);
        const QString pctStr = percentageToString(linearValue);
        return QString("%1 / %2").arg(pctStr).arg(freqStr);
    }
    if (suffix == "%") {
        return percentageToString(unmap(mappedValue, type, min, max) * Constants::uiInternalScaling());
    }
    if (suffix == "dB") {
        return decibelToString(unmap(mappedValue, type, min, max) * Constants::uiInternalScaling());
    }
    
    // Default to time-like formatting
    return timeToString(mappedValue, suffix);
}

double KnobController::mapIntensity(double value, double from, double to) const
{
    return ParameterMapper::mapCubicCentered(value, from, to);
}

double KnobController::unmapIntensity(double value, double from, double to) const
{
    return ParameterMapper::unmapCubicCentered(value, from, to);
}

QString KnobController::intensityToString(double value, double from, double to) const
{
    const double center = { (from + to) / 2.0 };
    const double range = { (to - from) / 2.0 };
    const double intVal = { range != 0 ? ((value - center) / range) * 100.0 : 0 };

    const QString sign = { intVal >= 0.05 ? "+" : "" };
    const double displayVal = { std::abs(intVal) < 0.05 ? 0.0 : intVal };
    return QString { "%1%2%" }.arg(sign).arg(displayVal, 0, 'f', 1);
}

double KnobController::mapPan(double value, double from, double to) const
{
    return mapIntensity(value, from, to);
}

double KnobController::unmapPan(double value, double from, double to) const
{
    return unmapIntensity(value, from, to);
}

QString KnobController::panToString(double value, double from, double to) const
{
    const double center = { (from + to) / 2.0 };
    const double range = { (to - from) / 2.0 };
    const double panVal = { range != 0 ? ((value - center) / range) * 100.0 : 0 };

    if (qFuzzyIsNull(panVal)) {
        return tr("Center");
    }

    const QString side = { panVal < 0 ? tr(" L") : tr(" R") };
    const QString sign = { panVal >= 0.05 ? "+" : (panVal <= -0.05 ? "-" : "") };
    return QString { "%1%2%%3" }.arg(sign).arg(std::abs(panVal), 0, 'f', 1).arg(side);
}

double KnobController::mapTime(double value, double from, double to) const
{
    return ParameterMapper::mapCubic(value, from, to);
}

double KnobController::unmapTime(double value, double from, double to) const
{
    return ParameterMapper::unmapCubic(value, from, to);
}

QString KnobController::timeToString(double value, const QString & suffix) const
{
    if (suffix == "s") {
        if (value < 1.0) {
            return QString("%1 ms").arg(std::round(value * 1000.0));
        }
        return QString("%1 s").arg(value, 0, 'f', 1);
    }
    return QString("%1%2").arg(std::round(value)).arg(suffix);
}

double KnobController::mapExponential(double value, double min, double max) const
{
    return ParameterMapper::mapExponential(value, min, max);
}

double KnobController::unmapExponential(double value, double min, double max) const
{
    return ParameterMapper::unmapExponential(value, min, max);
}

QString KnobController::percentageToString(double value) const
{
    const double displayValue = value / (Constants::uiInternalScaling() / 100.0);
    return QString("%1%").arg(displayValue, 0, 'f', 1);
}

QString KnobController::decibelToString(double value) const
{
    const double db = (value / Constants::uiInternalScaling() - 0.5) * 60.0;
    return QString("%1%2 dB").arg(db > 0 ? "+" : "").arg(db, 0, 'f', 1);
}

QString KnobController::frequencyToString(double value, double cutoffHz, bool isHpf) const
{
    if (value <= 0) {
        return "0 Hz";
    }
    if (!isHpf && value >= Constants::uiInternalScaling()) {
        return tr("Bypass");
    }
    if (cutoffHz >= 1000) {
        return QString("%1 kHz").arg(cutoffHz / 1000.0, 0, 'f', 1);
    }
    return QString("%1 Hz").arg(std::round(cutoffHz));
}

int KnobController::syncIndex(double value) const
{
    const double internalVal = value / Constants::uiInternalScaling();
    int bestIdx = 0;
    double minDiff = 10.0;
    for (size_t i = 0; i < syncDivisions.size(); ++i) {
        double diff = std::abs(syncDivisions[i] - internalVal);
        if (diff < minDiff) {
            minDiff = diff;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

double KnobController::syncValue(int index) const
{
    if (index < 0 || index >= static_cast<int>(syncDivisions.size())) return 0;
    return syncDivisions[static_cast<size_t>(index)] * Constants::uiInternalScaling();
}

QString KnobController::syncLabel(int index) const
{
    if (index < 0 || index >= static_cast<int>(syncLabels.size())) return "";
    return syncLabels[static_cast<size_t>(index)];
}

int KnobController::syncCount() const
{
    return static_cast<int>(syncDivisions.size());
}

} // namespace noteahead
