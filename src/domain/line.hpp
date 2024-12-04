#ifndef LINE_HPP
#define LINE_HPP

#include <memory>

namespace cacophony {

class Event;

class Line
{
public:
    Line();

private:
    std::shared_ptr<Event> m_event;

    uint8_t m_volume = 0;

    uint8_t m_panning = 0;
};

} // namespace cacophony

#endif // LINE_HPP
