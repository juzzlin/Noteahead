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

#ifndef XY_PAD_RENDERER_HPP
#define XY_PAD_RENDERER_HPP

#include <QColor>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QTimer>

#include <chrono>

namespace noteahead {

class XyPadController;

//! One pad of an XY pad device, played with the mouse: a press puts the hand on the pad, a drag
//! moves it and a release lets the pad glide home. The crosshair shows where the audio thread has
//! the pad, not where the hand is, so the glide is seen as it is heard. Redraws at the display rate
//! only while the pad is held or still on its way; otherwise it is idle.
class XyPadRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! The XyPadController to play. A QObject, since QML drops a forward-declared pointer type.
    Q_PROPERTY(QObject * controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(int pad READ pad WRITE setPad NOTIFY padChanged)
    //! What each axis moves, written along its edge. Empty for none.
    Q_PROPERTY(QString xLabel READ xLabel WRITE setXLabel NOTIFY xLabelChanged)
    Q_PROPERTY(QString yLabel READ yLabel WRITE setYLabel NOTIFY yLabelChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit XyPadRenderer(QQuickItem * parent = nullptr);

    QObject * controller() const;
    void setController(QObject * controller);

    int pad() const;
    void setPad(int pad);

    QString xLabel() const;
    void setXLabel(const QString & label);

    QString yLabel() const;
    void setYLabel(const QString & label);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void controllerChanged();
    void padChanged();
    void xLabelChanged();
    void yLabelChanged();
    void accentColorChanged();

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseUngrabEvent() override;
    void itemChange(ItemChange change, const ItemChangeData & data) override;

private:
    //! @p position in the item, as the pad's -1..1 with y up.
    QPointF toPad(const QPointF & position) const;
    void handOn(const QPointF & position);
    void letGo();
    void onFrame();
    //! Whether @p pad is the pad this renderer draws.
    bool showsPad(int pad) const;
    //! Redraws every frame from now on, until onFrame() finds the pad at rest. Does nothing while
    //! already redrawing.
    void startRedrawing();
    void onPadPressed(int pad);
    void onPadsAutomated(qint64 delayMs);

    QPointer<XyPadController> m_controller;
    int m_pad { 0 };
    //! Until when an automation is known to move the pad: redraws go on at least as long.
    std::chrono::steady_clock::time_point m_automatedUntil;
    QString m_xLabel;
    QString m_yLabel;
    QColor m_accentColor { "#4fc3f7" };
    bool m_handOn { false };
    QTimer m_frameTimer;
};

} // namespace noteahead

#endif // XY_PAD_RENDERER_HPP
