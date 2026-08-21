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

#ifndef TIP_SERVICE_HPP
#define TIP_SERVICE_HPP

#include <QObject>
#include <QString>

namespace noteahead {

//! The keyboard guidance shown in the left half of the bottom bar.
//!
//! One fixed tip per context: it changes when the state changes and at no other time, deliberately.
//! A tip that rotated on a timer would keep moving in the corner of the eye of someone trying to
//! work, and the bar's whole purpose is to stay still while the notification half does the moving.
//!
//! The edit mode is pushed in rather than read from ApplicationService, so that the tips can be
//! tested without one and so that this stays a lookup rather than a dependency.
class TipService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentTip READ currentTip NOTIFY currentTipChanged)

public:
    explicit TipService(QObject * parent = nullptr);

    QString currentTip() const;

public slots:
    void setEditMode(bool editMode);

signals:
    void currentTipChanged();

private:
    bool m_editMode = false;
};

} // namespace noteahead

#endif // TIP_SERVICE_HPP
