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

#ifndef RTAUDIO_COMPAT_HPP
#define RTAUDIO_COMPAT_HPP

#include "../../audio_recorder.hpp"

#include <RtAudio.h>

#include <stdexcept>
#include <string>
#include <vector>

//! The two places RtAudio 6 broke with RtAudio 5, which is what the current Ubuntu LTS still ships:
//! how devices are enumerated, and how a stream call reports failure. Everything else the player and
//! the recorder touch is the same in both.
//!
//! RtAudio 5 does not define RTAUDIO_VERSION_MAJOR at all, so a version that does not have it is a 5.
namespace noteahead::RtAudioCompat {

//! Every device \p accept says yes to, as the id to open it by and the name to show for it.
//!
//! RtAudio 6 hands out ids that are not the numbers 0..count-1, and asking it about an index it does
//! not recognise as an id answers with an empty device and a warning on the console -- which reads
//! as a machine with no devices on it at all. RtAudio 5 has no ids: there the index is what a stream
//! is opened by, so the two have to be asked differently.
template<typename Accept>
std::vector<AudioDevice> devices(RtAudio & rtAudio, Accept && accept)
{
    std::vector<AudioDevice> devices;
#if defined(RTAUDIO_VERSION_MAJOR) && RTAUDIO_VERSION_MAJOR >= 6
    for (const auto deviceId : rtAudio.getDeviceIds()) {
#else
    for (uint32_t deviceId = 0; deviceId < rtAudio.getDeviceCount(); deviceId++) {
#endif
        if (const auto info = rtAudio.getDeviceInfo(deviceId); accept(info)) {
            devices.push_back({ deviceId, info.name });
        }
    }
    return devices;
}

//! Runs one of RtAudio's stream calls and throws std::runtime_error carrying RtAudio's own message
//! when it fails.
//!
//! RtAudio 6 reports failure by return value rather than by throwing, so a try/catch around these
//! calls never fires and a stream that would not open plays or records silence without saying why.
//! RtAudio 5 throws and returns nothing at all, so neither half can be written for both.
template<typename Operation>
void checkedCall(RtAudio & rtAudio, Operation && operation, const std::string & what)
{
#if defined(RTAUDIO_VERSION_MAJOR) && RTAUDIO_VERSION_MAJOR >= 6
    if (const auto error = operation(); error != RTAUDIO_NO_ERROR) {
        throw std::runtime_error { what + ": " + rtAudio.getErrorText() };
    }
#else
    (void)rtAudio;
    try {
        operation();
    } catch (const RtAudioError & error) {
        throw std::runtime_error { what + ": " + error.getMessage() };
    }
#endif
}

} // namespace noteahead::RtAudioCompat

#endif // RTAUDIO_COMPAT_HPP
