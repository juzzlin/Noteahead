#ifndef MIDI_SERVICE_HPP
#define MIDI_SERVICE_HPP

#include <memory>
#include <rtmidi/RtMidi.h>

namespace cacophony {

class MidiService
{
public:
    MidiService();

    void listDevices() const;

    bool openDevice(uint32_t index);

    void sendNoteOn(uint32_t channel, uint32_t note, uint32_t velocity) const;

    void sendNoteOff(uint32_t channel, uint32_t note, uint32_t velocity) const;

private:
    std::unique_ptr<RtMidiOut> midiOut;
};

} // namespace cacophony

#endif // MIDI_SERVICE_HPP
