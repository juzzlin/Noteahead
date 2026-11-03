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

#include "drum_hit_preview.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead::DrumHitPreview {

namespace {

//! Long enough for the longest Decay any of the drums has, with room to spare
constexpr double MaxPreviewSeconds { 6.0 };

//! Where the hit counts as having gone quiet, so the picture is not mostly silence
constexpr double SilenceFloor { 1.0e-4 };

constexpr uint32_t BlockFrames { 256 };

std::unique_ptr<Device> clone(const Device & device, const CopyFactory & createCopy)
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

    auto copy = createCopy();
    if (copy) {
        copy->deserializeFromXml(reader);
        // The hit is drawn at the note it is asked for, not shifted by the device's octave
        copy->setOctave(0);
    }
    return copy;
}

std::vector<double> renderHit(Device & device, uint8_t note, double sampleRate)
{
    device.resetAudio();
    device.processMidiNoteOn(note, 127);

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

//! Frames up to and including the last one above the silence floor
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

Preview render(const Device & device, const CopyFactory & createCopy, const EnvelopeBypass & bypassEnvelope, uint8_t note, int peakCount, double sampleRate)
{
    if (peakCount <= 0 || sampleRate <= 0.0) {
        return {};
    }

    const auto copy = clone(device, createCopy);
    if (!copy) {
        return {};
    }

    Preview preview;

    // As it is set, first: this is the one that says how long the drum is heard for
    const auto heard = renderHit(*copy, note, sampleRate);
    preview.audibleSeconds = static_cast<double>(audibleFrames(heard)) / sampleRate;

    // Then without the envelope, for the picture the overlay is drawn over
    bypassEnvelope(*copy);
    const auto bare = renderHit(*copy, note, sampleRate);
    const auto frameCount = audibleFrames(bare);
    preview.durationSeconds = static_cast<double>(frameCount) / sampleRate;
    preview.peaks = toPeaks(bare, frameCount, peakCount);

    return preview;
}

} // namespace noteahead::DrumHitPreview
