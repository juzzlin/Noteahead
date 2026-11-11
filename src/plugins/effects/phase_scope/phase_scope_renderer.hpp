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

#ifndef PHASE_SCOPE_RENDERER_HPP
#define PHASE_SCOPE_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>

namespace noteahead {

//! Two waves over the same span, one on top of the other: the reference in grey and the signal in
//! the accent colour. Scaled together, by whichever is the larger, so that their sizes compare as
//! well as their shapes -- which is the whole point of laying one over the other.
class PhaseScopeRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVariantList reference READ reference WRITE setReference NOTIFY referenceChanged)
    Q_PROPERTY(QVariantList trace READ trace WRITE setTrace NOTIFY traceChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit PhaseScopeRenderer(QQuickItem * parent = nullptr);

    QVariantList reference() const;
    void setReference(const QVariantList & reference);
    QVariantList trace() const;
    void setTrace(const QVariantList & trace);
    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void referenceChanged();
    void traceChanged();
    void accentColorChanged();

private:
    QVariantList m_reference;
    QVariantList m_trace;
    QColor m_accentColor { Qt::green };
};

} // namespace noteahead

#endif // PHASE_SCOPE_RENDERER_HPP
