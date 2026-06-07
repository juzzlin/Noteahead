#ifndef EVENT_DATA_HPP
#define EVENT_DATA_HPP

#include <cstddef>

namespace noteahead {

class EventData
{
public:
    EventData(size_t track, size_t column);
    EventData();

    size_t track() const;
    void setTrack(size_t track);

    size_t column() const;
    void setColumn(size_t column);

private:
    size_t m_track = 0;
    size_t m_column = 0;
};

} // namespace noteahead

#endif // EVENT_DATA_HPP
