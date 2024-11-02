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
    bool openDevice(unsigned int index);
    void playMiddleC() const;

private:
    std::unique_ptr<RtMidiOut> midiOut;
};

} // namespace cacophony

#endif // MIDI_SERVICE_HPP
