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

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QObject>
#include <memory>

namespace cacophony {

class Song;

class Player : public QObject
{
    Q_OBJECT

public:
    Player();

    void play();

    void setSong(std::shared_ptr<Song> song);

private:
    std::shared_ptr<Song> m_song;
};

} // namespace cacophony

#endif // PLAYER_HPP
