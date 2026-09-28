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

#include "drum_voice_preview.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace noteahead::DrumVoicePreview {

namespace {

//! Longest a voice is followed before it is given up on. A cymbal with a long insert reverb on it
//! would otherwise be rendered for as long as the tail takes to fall under the floor, which on a
//! freeze is never.
constexpr double MaxPreviewSeconds { 8.0 };

//! Below this a frame counts as silence when the audible length is measured. Well under anything
//! that can be heard over a mix, and well over the denormals a filter leaves behind.
constexpr double SilenceFloor { 1.0e-4 };

//! An independent copy of the device, made the only way a device can be copied: through the format
//! it is saved in.
std::unique_ptr<DrumSynthV2Device> clone(const DrumSynthV2Device & device)
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

    auto copy = std::make_unique<DrumSynthV2Device>(device.name());
    copy->deserializeFromXml(reader);
    return copy;
}

//! Where the sound actually stops, in seconds, ignoring the silence a block boundary leaves behind.
double audibleLength(const std::vector<double> & frames, double sampleRate)
{
    for (size_t i = frames.size(); i > 0; i--) {
        if (std::abs(frames.at(i - 1)) > SilenceFloor) {
            return static_cast<double>(i / 2) / sampleRate;
        }
    }
    return 0.0;
}

//! The loudest sample in each of @p peakCount equal slices, which is what the view draws.
QVariantList toPeaks(const std::vector<double> & frames, int peakCount)
{
    QVariantList peaks;
    const auto frameCount = frames.size() / 2;
    if (!frameCount || peakCount <= 0) {
        return peaks;
    }

    peaks.reserve(peakCount);
    for (int point = 0; point < peakCount; point++) {
        const auto from = frameCount * static_cast<size_t>(point) / static_cast<size_t>(peakCount);
        const auto to = std::max(from + 1, frameCount * static_cast<size_t>(point + 1) / static_cast<size_t>(peakCount));
        double peak = 0.0;
        for (auto frame = from; frame < to && frame < frameCount; frame++) {
            peak = std::max({ peak, std::abs(frames.at(frame * 2)), std::abs(frames.at(frame * 2 + 1)) });
        }
        peaks.append(std::min(1.0, peak));
    }
    return peaks;
}

} // namespace

Preview render(const DrumSynthV2Device & device, int voiceIndex, int peakCount, double sampleRate)
{
    if (peakCount <= 0 || sampleRate <= 0.0) {
        return {};
    }

    const auto copy = clone(device);
    if (!copy) {
        return {};
    }

    Preview preview;

    // As it is set, first: this is the one that says how long the drum is heard for, effects and
    // all, and it has to be measured before the envelope is switched off.
    const auto heard = copy->renderVoiceAlone(voiceIndex, sampleRate, MaxPreviewSeconds);
    preview.audibleSeconds = audibleLength(heard, sampleRate);

    // Then with the envelope switched off, for the picture the overlay is drawn over. A sustain at
    // full and nothing else in the way is an envelope that does nothing at all, which is exactly
    // what a Sampler pad's untouched file is.
    copy->updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyAmpAttack().toStdString(), 0.0f);
    copy->updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyAmpHold().toStdString(), 0.0f);
    copy->updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.0f);
    copy->updateVoiceParameter(voiceIndex, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 1.0f);

    const auto bare = copy->renderVoiceAlone(voiceIndex, sampleRate, MaxPreviewSeconds);
    preview.durationSeconds = static_cast<double>(bare.size() / 2) / sampleRate;
    preview.peaks = toPeaks(bare, peakCount);

    return preview;
}

} // namespace noteahead::DrumVoicePreview
