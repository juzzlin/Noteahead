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
  : QObject { parent }
{
}

double KnobController::map(double value, const QString & type, double min, double max) const
{
    if (type == "exponential")
        return ParameterMapper::mapExponential(value, min, max);
    if (type == "cubic")
        return ParameterMapper::mapCubic(value, min, max);
    if (type == "cubicCentered" || type == "pan" || type == "intensity")
        return ParameterMapper::mapCubicCentered(value * 2.0 - 1.0, min, max);
    if (type == "logFrequency")
        return ParameterMapper::mapLogFrequency(value, min, max);
    if (type == "decibel")
        return ParameterMapper::mapDecibel(value, (max - min) / 2.0);
    return min + (value * (max - min)); // linear
}

double KnobController::unmap(double mappedValue, const QString & type, double min, double max) const
{
    if (type == "exponential")
        return ParameterMapper::unmapExponential(mappedValue, min, max);
    if (type == "cubic")
        return ParameterMapper::unmapCubic(mappedValue, min, max);
    if (type == "cubicCentered" || type == "pan" || type == "intensity")
        return (ParameterMapper::unmapCubicCentered(mappedValue, min, max) + 1.0) / 2.0;
    if (type == "logFrequency")
        return ParameterMapper::unmapLogFrequency(mappedValue, min, max);
    if (type == "decibel")
        return ParameterMapper::unmapDecibel(mappedValue, (max - min) / 2.0);
    return (max != min) ? (mappedValue - min) / (max - min) : 0.0;
}

QString KnobController::format(double value, const QString & type, const QString & suffix, double min, double max) const
{
    if (type == "pan") {
        return panToString(value, 0.0, 1.0);
    }

    if (type == "volume") {
        const double linearValue = value * Constants::uiInternalScaling();
        const QString pctStr = percentageToString(linearValue);
        const QString dbStr = decibelMultiplierToString(value);
        return QString { "%1 / %2" }.arg(pctStr).arg(dbStr);
    }

    const double mappedValue = map(value, type, min, max);

    if (type == "intensity" || type == "cubicCentered") {
        return bipolarToString(mappedValue, suffix, min, max);
    }
    if (type == "logFrequency" || type == "frequency") {
        const double linearValue = value * Constants::uiInternalScaling();
        const QString freqStr = frequencyToString(linearValue, mappedValue, false);
        const QString pctStr = percentageToString(linearValue);
        return QString { "%1 / %2" }.arg(pctStr).arg(freqStr);
    }
    if (type == "decibel") {
        return decibelMultiplierToString(mappedValue);
    }
    if (type == "exponential" && suffix.isEmpty()) {
        return QString { "%1" }.arg(mappedValue, 0, 'f', 2);
    }
    if (suffix == "%") {
        return percentageToString(value * Constants::uiInternalScaling());
    }
    if (suffix == "dB") {
        return decibelToString(mappedValue, min, max);
    }

    // Default to time-like formatting
    return timeToString(mappedValue, suffix);
}

QString KnobController::bipolarToString(double value, const QString & suffix, double /*from*/, double /*to*/) const
{
    const QString sign = { value >= 0.05 ? "+" : (value <= -0.05 ? "-" : "") };
    const double absVal = { std::abs(value) < 0.05 ? 0.0 : std::abs(value) };
    return QString { "%1%2%3" }.arg(sign).arg(absVal, 0, 'f', 1).arg(suffix);
}

double KnobController::mapPan(double value, double from, double to) const
{
    return ParameterMapper::mapCubicCentered(value, from, to);
}

double KnobController::unmapPan(double value, double from, double to) const
{
    return ParameterMapper::unmapCubicCentered(value, from, to);
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
    if (suffix == "s" || suffix == "ms") {
        double seconds = (suffix == "s") ? value : value / 1000.0;
        if (seconds < 0.000001) {
            return tr("0 ms");
        }
        if (seconds < 0.0001) {
            return QString { "%1 μs" }.arg(std::round(seconds * 1000000.0));
        }
        if (seconds < 1.0) {
            const double ms = seconds * 1000.0;
            return QString { "%1 ms" }.arg(ms, 0, 'f', 1);
        }
        return QString { "%1 s" }.arg(seconds, 0, 'f', 1);
    }
    return QString { "%1 %2" }.arg(value, 0, 'f', 1).arg(suffix);
}

double KnobController::mapExponential(double value, double min, double max) const
{
    return ParameterMapper::mapExponential(value, min, max);
}

double KnobController::unmapExponential(double value, double min, double max) const
{
    return ParameterMapper::unmapExponential(value, min, max);
}

QString KnobController::percentageToString(double value, double from, double to) const
{
    const double range = to - from;
    const double pct = { range != 0 ? ((value - from) / range) * 100.0 : 0 };
    return QString { "%1%" }.arg(pct, 0, 'f', 1);
}

QString KnobController::decibelToString(double value, double from, double to) const
{
    double db;
    if (from == 0 && to == Constants::uiInternalScaling()) {
        // Legacy: value is 0..1000, map to -30..30 dB
        db = (value / Constants::uiInternalScaling() - 0.5) * 60.0;
    } else {
        // Generic case: value is already in dB
        db = value;
    }
    const QString sign = { db >= 0.05 ? "+" : (db <= -0.05 ? "-" : "") };
    return QString { "%1%2 dB" }.arg(sign).arg(std::abs(db), 0, 'f', 1);
}

QString KnobController::decibelMultiplierToString(double multiplier) const
{
    if (multiplier <= 0)
        return "-inf dB";
    const double db = 20.0 * std::log10(multiplier);
    const QString sign = { db >= 0.05 ? "+" : (db <= -0.05 ? "-" : "") };
    return QString { "%1%2 dB" }.arg(sign).arg(std::abs(db), 0, 'f', 1);
}

QString KnobController::valueToString(double value, const QString & suffix, double from, double to, bool isInteger) const
{
    if (suffix == "%") {
        return percentageToString(value, from, to);
    }
    if (suffix == "dB") {
        return decibelToString(value, from, to);
    }
    if (suffix == "Hz") {
        return QString::number(static_cast<int>(std::round(value))) + "Hz";
    }
    if (suffix == "s" || suffix == "ms") {
        return timeToString(value, suffix);
    }
    return QString::number(isInteger ? std::round(value) : value, 'f', isInteger ? 0 : 2) + suffix;
}

QString KnobController::frequencyToString(double linearValue, double mappedValue, bool forceHz) const
{
    if (!forceHz && linearValue >= Constants::uiInternalScaling() - 0.1) {
        return tr("Bypass");
    }
    if (mappedValue < 1000.0) {
        return QString { "%1 Hz" }.arg(std::round(mappedValue));
    }
    return QString { "%1 kHz" }.arg(mappedValue / 1000.0, 0, 'f', 1);
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
    if (index < 0 || index >= static_cast<int>(syncDivisions.size()))
        return 0;
    return syncDivisions[static_cast<size_t>(index)] * Constants::uiInternalScaling();
}

QString KnobController::syncLabel(int index) const
{
    if (index < 0 || index >= static_cast<int>(syncLabels.size()))
        return "";
    return syncLabels[static_cast<size_t>(index)];
}

int KnobController::syncCount() const
{
    return static_cast<int>(syncDivisions.size());
}

} // namespace noteahead
