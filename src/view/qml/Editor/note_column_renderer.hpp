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

#ifndef NOTE_COLUMN_RENDERER_HPP
#define NOTE_COLUMN_RENDERER_HPP

#include <QColor>
#include <QList>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariantList>

#include <optional>
#include <utility>
#include <vector>

class QAbstractListModel;

namespace noteahead {

class NoteColumnRenderer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel * model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(double scrollOffset READ scrollOffset WRITE setScrollOffset NOTIFY scrollOffsetChanged)
    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)
    //! See Constants::AutomationDisplayMode. Curve draws each automation as its own vertical trace.
    Q_PROPERTY(int automationDisplayMode READ automationDisplayMode WRITE setAutomationDisplayMode NOTIFY automationDisplayModeChanged)
    //! Width of a drawn automation curve, in tenths of a pixel.
    Q_PROPERTY(int automationCurveThicknessTenths READ automationCurveThicknessTenths WRITE setAutomationCurveThicknessTenths NOTIFY automationCurveThicknessTenthsChanged)
    //! Hue of the edit cursor. Drawn half-transparent, so the note under the cursor stays readable.
    Q_PROPERTY(QColor cursorColor READ cursorColor WRITE setCursorColor NOTIFY cursorColorChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(QColor textColorEmpty READ textColorEmpty WRITE setTextColorEmpty NOTIFY textColorEmptyChanged)
    Q_PROPERTY(QColor textColorGhost READ textColorGhost WRITE setTextColorGhost NOTIFY textColorGhostChanged)
    //! Colours the automation curves cycle through, so several automations on one column stay apart.
    Q_PROPERTY(QVariantList automationCurveColors READ automationCurveColors WRITE setAutomationCurveColors NOTIFY automationCurveColorsChanged)
    Q_PROPERTY(QColor automationCurveCenterLineColor READ automationCurveCenterLineColor WRITE setAutomationCurveCenterLineColor NOTIFY automationCurveCenterLineColorChanged)

public:
    explicit NoteColumnRenderer(QQuickItem * parent = nullptr);

    //! Contiguous spans of set values as inclusive [first, last] index pairs. A gap where the
    //! automation does not reach breaks the span, so it becomes a gap on screen too.
    //!
    //! A span of one is what a single-line automation collapses to, and also what a longer one
    //! clips to when only its first or last row is on screen. Those cannot be drawn as a polyline,
    //! which is why the runs are named here rather than being drawn as they are found.
    static std::vector<std::pair<size_t, size_t>> valueRuns(const std::vector<std::optional<double>> & values);

    QAbstractListModel * model() const;
    void setModel(QAbstractListModel * model);

    double scrollOffset() const;
    void setScrollOffset(double scrollOffset);

    int visibleLines() const;
    void setVisibleLines(int visibleLines);

    int automationDisplayMode() const;
    void setAutomationDisplayMode(int mode);

    int automationCurveThicknessTenths() const;
    void setAutomationCurveThicknessTenths(int tenths);

    QColor cursorColor() const;
    void setCursorColor(const QColor & cursorColor);

    QColor textColor() const;
    void setTextColor(const QColor & textColor);

    QColor textColorEmpty() const;
    void setTextColorEmpty(const QColor & textColorEmpty);

    QColor textColorGhost() const;
    void setTextColorGhost(const QColor & textColorGhost);

    QVariantList automationCurveColors() const;
    void setAutomationCurveColors(const QVariantList & automationCurveColors);

    QColor automationCurveCenterLineColor() const;
    void setAutomationCurveCenterLineColor(const QColor & automationCurveCenterLineColor);

    void paint(QPainter * painter) override;

signals:
    void modelChanged();
    void scrollOffsetChanged();
    void visibleLinesChanged();
    void automationDisplayModeChanged();
    void automationCurveThicknessTenthsChanged();
    void cursorColorChanged();
    void textColorChanged();
    void textColorEmptyChanged();
    void textColorGhostChanged();
    void automationCurveColorsChanged();
    void automationCurveCenterLineColorChanged();

private:
    QPointer<QAbstractListModel> m_model { nullptr };
    double m_scrollOffset { 0.0 };
    int m_visibleLines { 0 };
    int m_automationDisplayMode { 0 };
    int m_automationCurveThicknessTenths { 15 };
    QColor m_cursorColor { "red" };
    QColor m_textColor { "#ffffff" };
    QColor m_textColorEmpty { "#888888" };
    QColor m_textColorGhost { "#444444" };
    QList<QColor> m_automationCurveColors;
    QColor m_automationCurveCenterLineColor { "#808080" };
    //! Set until the requested full repaint has actually been painted. QQuickPaintedItem keeps a
    //! single dirty rect, so a partial update issued afterwards would silently shrink it to that row.
    bool m_fullRepaintPending { true };

    void paintAutomationCurves(QPainter * painter, int startRow, int endRow, qreal rowHeight);

    //! Marks the whole column dirty and remembers it until it has been painted.
    void requestFullRepaint();

    //! Repaints only the rows a dataChanged() covers. A cursor move touches one row, so it no
    //! longer costs a repaint and texture upload of the whole column.
    void updateRows(int firstRow, int lastRow);
};

} // namespace noteahead

#endif // NOTE_COLUMN_RENDERER_HPP
