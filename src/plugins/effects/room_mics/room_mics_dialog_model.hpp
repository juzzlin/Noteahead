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

#ifndef ROOM_MICS_DIALOG_MODEL_HPP
#define ROOM_MICS_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

//! What the Room Mics dialog needs beyond its parameters: where everything stands in the room, in
//! metres, for the room view, and the moves the view makes, each of which sets two parameters.
class RoomMicsDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit RoomMicsDialogModel(QObject * parent = nullptr);

    //! width, depth and height of the room; sourceCentre and sources, each an {x, y}; groups, each
    //! with centre, pattern, level and mics, each mic an {x, y, aimX, aimY}; and reverbTime, the
    //! mid-band RT60 in seconds, with reverbTimes per octave from 125 Hz. Empty for no such effect.
    Q_INVOKABLE QVariantMap layout() const;
    //! Puts the middle of the source at @p x, @p y metres.
    Q_INVOKABLE void moveSource(double x, double y);
    //! Puts the middle of mic pair @p group at @p x, @p y metres. The Close pair is moved by its
    //! offset from the source, which it keeps when the source moves.
    Q_INVOKABLE void moveMics(int group, double x, double y);
};

} // namespace noteahead

#endif // ROOM_MICS_DIALOG_MODEL_HPP
