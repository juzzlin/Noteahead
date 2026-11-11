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

#include "snare_synth_preview.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "../../../infra/xml/nahd_xml_reader.hpp"
#include "../../../infra/xml/nahd_xml_writer.hpp"
#include "snare_synth_device.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace noteahead::SnareSynthPreview {

namespace {

//! Longest a hit is followed. The longest the knobs make a snare ring is well under this.
constexpr double MaxPreviewSeconds { 4.0 };

//! Below this a frame counts as silence when the audible length is measured.
constexpr double SilenceFloor { 1.0e-4 };

constexpr uint32_t BlockFrames { 256 };

//! C-3: the note that plays the drum as it is set, Chromatic or not.
constexpr uint8_t PreviewNote { 36 };

std::unique_ptr<SnareSynthDevice> clone(const SnareSynthDevice & device)
{
    QString xml;
    {
        NahdXmlWriter writer { xml };
        device.serializeToXml(writer);
    }

    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    if (reader.atEnd() || reader.hasError()) {
        return {};
    }

    auto copy = std::make_unique<SnareSynthDevice>(device.name());
    copy->deserializeFromXml(reader);
    return copy;
}

//! One hit at full velocity, interleaved stereo, until the drum stops or the limit is reached.
std::vector<double> renderHit(SnareSynthDevice & device, double sampleRate)
{
    device.resetAudio();
    device.processDeviceMidiNoteOn(PreviewNote, 127);

    const auto maxFrames = static_cast<size_t>(MaxPreviewSeconds * sampleRate);
    std::vector<double> frames;
    std::vector<double> block(BlockFrames * 2);
    while (frames.size() / 2 < maxFrames && device.hasActiveAudio()) {
        std::ranges::fill(block, 0.0);
        AudioContext context {
            .buffer = block,
            .frameCount = BlockFrames,
            .sampleRate = static_cast<uint32_t>(sampleRate),
        };
        device.processAudio(context);
        frames.insert(frames.end(), block.begin(), block.end());
    }
    return frames;
}

double audibleLength(const std::vector<double> & frames, double sampleRate)
{
    for (size_t i = frames.size(); i > 0; i--) {
        if (std::abs(frames.at(i - 1)) > SilenceFloor) {
            return static_cast<double>(i / 2) / sampleRate;
        }
    }
    return 0.0;
}

//! Where the drum has fallen under the floor for good: the picture stops there rather than at the
//! end of the engine's inaudible tail.
size_t audibleFrames(const std::vector<double> & frames)
{
    for (size_t i = frames.size(); i > 0; i--) {
        if (std::abs(frames.at(i - 1)) > SilenceFloor) {
            return i / 2 + 1;
        }
    }
    return 0;
}

QVariantList toPeaks(const std::vector<double> & frames, size_t frameCount, int peakCount)
{
    QVariantList peaks;
    if (!frameCount || peakCount <= 0) {
        return peaks;
    }

    peaks.reserve(peakCount);
    for (int point = 0; point < peakCount; point++) {
        const auto from = frameCount * static_cast<size_t>(point) / static_cast<size_t>(peakCount);
        const auto to = std::max(from + 1, frameCount * static_cast<size_t>(point + 1) / static_cast<size_t>(peakCount));
        double peak = 0.0;
        for (auto frame = from; frame < to && frame * 2 + 1 < frames.size(); frame++) {
            peak = std::max({ peak, std::abs(frames.at(frame * 2)), std::abs(frames.at(frame * 2 + 1)) });
        }
        peaks.append(std::min(1.0, peak));
    }
    return peaks;
}

} // namespace

Preview render(const SnareSynthDevice & device, int peakCount, double sampleRate)
{
    if (peakCount <= 0 || sampleRate <= 0.0) {
        return {};
    }

    const auto copy = clone(device);
    if (!copy) {
        return {};
    }

    Preview preview;

    const auto heard = renderHit(*copy, sampleRate);
    preview.audibleSeconds = audibleLength(heard, sampleRate);

    // Then with the envelope left out, for the picture it is drawn over
    copy->setAmpAttack(0.0f);
    copy->setAmpSustain(1.0f);
    copy->setAmpRelease(0.0f);
    const auto bare = renderHit(*copy, sampleRate);
    const auto frameCount = audibleFrames(bare);
    preview.durationSeconds = static_cast<double>(frameCount) / sampleRate;
    preview.peaks = toPeaks(bare, frameCount, peakCount);

    return preview;
}

} // namespace noteahead::SnareSynthPreview
