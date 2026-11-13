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

#ifndef ROOM_VIEW_RENDERER_HPP
#define ROOM_VIEW_RENDERER_HPP

#include <QColor>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace noteahead {

//! The room of the Room Mics effect seen from above, front wall at the top: the source as a pair
//! of speakers, the mic pairs with their polar patterns turned the way they face, and the direct
//! paths between them. The source and the pairs can be dragged; where they are dropped is
//! signalled in metres, and what to do with it is left to the dialog.
//!
//! A drag on a speaker moves the source, its Close mics with it. The Close mics stand on the
//! speakers until moved, so Shift picks them up instead; once off the speakers they can be picked
//! up directly.
class RoomViewRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    //! RoomMicsDialogModel::layout()
    Q_PROPERTY(QVariantMap layout READ layout WRITE setLayout NOTIFY layoutChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! What the source is called: the device the effect is on.
    Q_PROPERTY(QString sourceName READ sourceName WRITE setSourceName NOTIFY sourceNameChanged)
    //! The pairs' names, translated, Close first.
    Q_PROPERTY(QStringList groupNames READ groupNames WRITE setGroupNames NOTIFY groupNamesChanged)

public:
    explicit RoomViewRenderer(QQuickItem * parent = nullptr);

    QVariantMap layout() const;
    void setLayout(const QVariantMap & layout);
    QColor accentColor() const;
    void setAccentColor(const QColor & color);
    QString sourceName() const;
    void setSourceName(const QString & name);
    QStringList groupNames() const;
    void setGroupNames(const QStringList & names);

    void paint(QPainter * painter) override;

signals:
    void layoutChanged();
    void accentColorChanged();
    void sourceNameChanged();
    void groupNamesChanged();

    //! The middle of the source dragged to @p x, @p y metres.
    void sourceDragged(double x, double y);
    //! The middle of pair @p group dragged to @p x, @p y metres.
    void micsDragged(int group, double x, double y);

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseUngrabEvent() override;
    void hoverMoveEvent(QHoverEvent * event) override;

private:
    //! What is under the pointer: -1 for the source, a pair's index otherwise.
    struct Target
    {
        int item { -1 };
        //! Where the item's middle is, in metres.
        QPointF centre;
    };

    //! The room's rectangle on screen, and the pixels to a metre.
    QRectF roomRect() const;
    double pixelsPerMetre() const;
    QPointF toPixels(const QPointF & metres) const;
    QPointF toMetres(const QPointF & pixels) const;
    std::optional<Target> hitTest(const QPointF & pixels, bool preferCloseMics) const;
    QColor groupColor(int group) const;

    QVariantMap m_layout;
    QColor m_accentColor { 0x4c, 0xaf, 0x50 };
    QString m_sourceName;
    QStringList m_groupNames;

    std::optional<Target> m_drag;
    //! Where in the item the pointer took hold of it, so a drag does not snap its middle under it.
    QPointF m_grabOffset;
};

} // namespace noteahead

#endif // ROOM_VIEW_RENDERER_HPP
