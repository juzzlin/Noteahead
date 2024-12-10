// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef EDITOR_SERVICE_HPP
#define EDITOR_SERVICE_HPP

#include "position.hpp"
#include <QObject>

namespace cacophony {

class Song;

class EditorService : public QObject
{
    Q_OBJECT

public:
    EditorService();

    void initialize();

    using SongS = std::shared_ptr<Song>;

    void setSong(SongS song);

    Q_INVOKABLE uint32_t columnCount(uint32_t trackId) const;

    Q_INVOKABLE uint32_t lineCount(uint32_t patternId) const;

    Q_INVOKABLE uint32_t currentLineCount() const;

    Q_INVOKABLE int lineNumberAtViewLine(uint32_t line) const;

    Q_INVOKABLE uint32_t linesVisible() const;

    Q_INVOKABLE QString noteAtPosition(uint32_t patternId, uint32_t trackId, uint32_t columnId, uint32_t line) const;

    Q_INVOKABLE uint32_t patternCount() const;

    Q_INVOKABLE uint32_t trackCount() const;

    Q_INVOKABLE QString trackName(uint32_t trackId) const;

    Q_INVOKABLE void setTrackName(uint32_t trackId, QString name);

    Q_INVOKABLE uint32_t currentPatternId() const;

    Q_INVOKABLE void setCurrentPatternId(uint32_t currentPatternId);

    Q_INVOKABLE Position position() const;

    Q_INVOKABLE uint32_t positionBarLine() const;

    Q_INVOKABLE void requestScroll(int steps);

    Q_INVOKABLE void requestTrackFocus(uint32_t trackId);

signals:
    void currentPatternChanged();

    void positionChanged(const Position & position);

    void songChanged();

private:
    SongS m_song;

    uint32_t m_currentPatternId = 0;

    Position m_position;
};

} // namespace cacophony

#endif // EDITOR_SERVICE_HPP
