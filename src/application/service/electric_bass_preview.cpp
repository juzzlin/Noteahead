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

#include "electric_bass_preview.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/electric_bass_device.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace noteahead::ElectricBassPreview {

namespace {

//! How long the note is held: long enough for the Sustain and Brightness knobs to show in its
//! decay, short enough for the release to be seen after it
constexpr double HoldSeconds { 2.0 };

//! Longest the release is followed
constexpr double MaxReleaseSeconds { 1.5 };

//! Below this a frame counts as silence
constexpr double SilenceFloor { 1.0e-4 };

constexpr uint32_t BlockFrames { 256 };

//! The open E, the lowest string of a 4-string
constexpr uint8_t PreviewNote { 28 };

std::unique_ptr<ElectricBassDevice> clone(const ElectricBassDevice & device)
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

    auto copy = std::make_unique<ElectricBassDevice>(device.name());
    copy->deserializeFromXml(reader);
    return copy;
}

void renderFor(ElectricBassDevice & device, double sampleRate, size_t frameCount, std::vector<double> & frames)
{
    std::vector<double> block(BlockFrames * 2);
    const auto end = frames.size() / 2 + frameCount;
    while (frames.size() / 2 < end && device.hasActiveAudio()) {
        std::ranges::fill(block, 0.0);
        AudioContext context {
            .buffer = block,
            .frameCount = BlockFrames,
            .sampleRate = static_cast<uint32_t>(sampleRate),
        };
        device.processAudio(context);
        frames.insert(frames.end(), block.begin(), block.end());
    }
}

//! The note, interleaved stereo: held, then let go until it dies or the limit is reached.
std::vector<double> renderNote(ElectricBassDevice & device, double sampleRate)
{
    device.resetAudio();
    std::vector<double> frames;
    device.processDeviceMidiNoteOn(PreviewNote, 127);
    renderFor(device, sampleRate, static_cast<size_t>(HoldSeconds * sampleRate), frames);
    device.processDeviceMidiNoteOff(PreviewNote);
    renderFor(device, sampleRate, static_cast<size_t>(MaxReleaseSeconds * sampleRate), frames);
    return frames;
}

//! Where the note has fallen under the floor for good: the picture stops there.
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

Preview render(const ElectricBassDevice & device, int peakCount, double sampleRate)
{
    if (peakCount <= 0 || sampleRate <= 0.0) {
        return {};
    }

    const auto copy = clone(device);
    if (!copy) {
        return {};
    }

    const auto frames = renderNote(*copy, sampleRate);
    const auto frameCount = audibleFrames(frames);
    return {
        .peaks = toPeaks(frames, frameCount, peakCount),
        .durationSeconds = static_cast<double>(frameCount) / sampleRate,
    };
}

} // namespace noteahead::ElectricBassPreview
