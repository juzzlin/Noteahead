#include "event_data.hpp"

namespace noteahead {

EventData::EventData(size_t track, size_t column)
  : m_track { track }
  , m_column { column }
{
}

EventData::EventData() = default;

size_t EventData::track() const
{
    return m_track;
}

void EventData::setTrack(size_t track)
{
    m_track = track;
}

size_t EventData::column() const
{
    return m_column;
}

void EventData::setColumn(size_t column)
{
    m_column = column;
}

} // namespace noteahead
