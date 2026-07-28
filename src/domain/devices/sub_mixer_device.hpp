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

#ifndef SUB_MIXER_DEVICE_HPP
#define SUB_MIXER_DEVICE_HPP

#include "device.hpp"

#include <string>
#include <vector>

namespace noteahead {

//! Groups other devices so a whole set can be processed and mixed as one entity.
//!
//! The SubMixer takes no MIDI. It sums the outputs of its member devices and then behaves exactly
//! like any other device: its insert effect rack, level, pan and sends apply to the group as a
//! whole, which is the point -- one compressor across the backing instead of one per device.
//!
//! Members are declared as sidechain dependencies, which is what makes this work at all: the audio
//! engine topologically sorts devices by those dependencies, so every member is guaranteed to have
//! rendered (including its own insert effects) before the SubMixer reads its output buffer. Nesting
//! a SubMixer inside another needs no extra code for the same reason.
//!
//! Members do not also reach the master directly. AudioEngine suppresses the direct contribution of
//! any device claimed by a SubMixer, so the group is heard once, through here.
class SubMixerDevice : public Device
{
public:
    explicit SubMixerDevice(std::string name);
    ~SubMixerDevice() override;

    std::string name() const override;
    std::string category() const override;
    std::string typeName() const override;
    std::string typeId() const override;

    static std::string typeIdString();

    //! Device slots whose output this SubMixer sums. Ownership of the rules that keep this list
    //! sane -- no cycles, no device in two SubMixers, no stale slots -- belongs to DeviceService.
    using SlotList = std::vector<size_t>;
    SlotList members() const;
    void setMembers(SlotList members);

    //! Volume and Pan, so the group can be ridden from a track the way any device can.
    //! Notes are ignored: there is nothing here to play.
    std::vector<MidiCcController> availableMidiCcControllers() const override;

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
    void processMidiAllNotesOff() override;

    void processAudio(AudioContext & context) override;
    std::vector<size_t> claimedOutputSlots() const override;
    std::vector<size_t> sidechainDependencies() const override;
    void sidechainDependencies(std::vector<size_t> & out) const override;

    //! Always true: a SubMixer cannot cheaply tell whether its members are silent, and reporting
    //! false would let the engine skip it while a member still had a tail running.
    bool hasActiveAudio() const override;

    void serializeToXml(ProjectWriter & writer) const override;
    void deserializeFromXml(ProjectReader & reader) override;

private:
    std::string m_name;
    SlotList m_members;
};

} // namespace noteahead

#endif // SUB_MIXER_DEVICE_HPP
