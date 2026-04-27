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

#include "waveform_generator.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <sndfile.h>
#include <algorithm>
#include <vector>

namespace noteahead::WaveformGenerator {

static const auto TAG = "WaveformGenerator";

QVariantList getWaveformData(const QString & filePath, int numPoints)
{
    if (filePath.isEmpty()) {
        return {};
    }

    SF_INFO sfInfo = {};
    SNDFILE * sndFile = sf_open(filePath.toStdString().c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        juzzlin::L(TAG).error() << "Could not open audio file: " << filePath.toStdString();
        return {};
    }

    if (sfInfo.frames <= 0 || numPoints <= 0) {
        sf_close(sndFile);
        return {};
    }

    QVariantList points;
    points.reserve(numPoints);

    const int channels = sfInfo.channels;
    const qint64 totalFrames = sfInfo.frames;
    const qint64 framesPerPoint = std::max(1ll, static_cast<long long>(totalFrames) / numPoints);
    std::vector<double> buffer(framesPerPoint * channels);

    for (int i = 0; i < numPoints; ++i) {
        const auto readFrames = sf_readf_double(sndFile, buffer.data(), framesPerPoint);
        if (readFrames <= 0) break;

        double maxVal = 0.0;
        for (int j = 0; j < readFrames * channels; ++j) {
            maxVal = std::max(maxVal, std::abs(buffer[j]));
        }
        points.append(maxVal);
    }

    sf_close(sndFile);
    return points;
}

} // namespace noteahead::WaveformGenerator
