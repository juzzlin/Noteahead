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

    Q_INVOKABLE uint32_t trackCount() const;

signals:
    void songChanged();

private:
    SongS m_song;
};

} // namespace cacophony

#endif // EDITOR_SERVICE_HPP
