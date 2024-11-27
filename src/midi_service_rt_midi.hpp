#ifndef MIDI_SERVICE_RT_MIDI_HPP
#define MIDI_SERVICE_RT_MIDI_HPP

#include "midi_service.hpp"

#include <memory>
#include <rtmidi/RtMidi.h>

namespace cacophony {

//! MIDI-service implementation on the RtMidi library.
class MidiServiceRtMidi : public MidiService
{
public:
    MidiServiceRtMidi();

    void listDevices() const override;

    bool openDevice(uint32_t index) override;

    void sendNoteOn(uint32_t channel, uint32_t note, uint32_t velocity) const override;

    void sendNoteOff(uint32_t channel, uint32_t note, uint32_t velocity) const override;

private:
    std::unique_ptr<RtMidiOut> midiOut;
};

} // namespace cacophony

#endif // MIDI_SERVICE_RT_MIDI_HPP
