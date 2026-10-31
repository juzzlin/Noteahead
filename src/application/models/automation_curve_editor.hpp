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

#ifndef AUTOMATION_CURVE_EDITOR_HPP
#define AUTOMATION_CURVE_EDITOR_HPP

#include "../../domain/tracker/automation_curve.hpp"

#include <QObject>
#include <QVariantList>

#include <vector>

namespace noteahead {

//! The curve the curve dialog is drawing, held here until the dialog is accepted.
//!
//! The dialog hands the result to whichever automations model it was opened from, as a node list,
//! so Cancel leaves the automation alone. A mouse gesture, from press to release, is one undo step:
//! dragging a node through a hundred positions is undone in one go, as it was made.
class AutomationCurveEditor : public QObject
{
    Q_OBJECT

    //! [{ line, value }, ...], sorted by line, the ends first and last.
    Q_PROPERTY(QVariantList nodes READ nodes NOTIFY nodesChanged)
    Q_PROPERTY(quint64 line0 READ line0 NOTIFY rangeChanged)
    Q_PROPERTY(quint64 line1 READ line1 NOTIFY rangeChanged)
    Q_PROPERTY(double minValue READ minValue NOTIFY rangeChanged)
    Q_PROPERTY(double maxValue READ maxValue NOTIFY rangeChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)

public:
    explicit AutomationCurveEditor(QObject * parent = nullptr);

    //! Starts editing @p nodes, or the ramp from @p value0 to @p value1 when there are none.
    //!
    //! Values are kept within @p minValue and @p maxValue, and on multiples of @p valueStep from
    //! @p minValue when it is positive: whole steps for a MIDI CC, free for a knob.
    Q_INVOKABLE void edit(const QVariantList & nodes, quint64 line0, quint64 line1, double value0, double value1, double minValue, double maxValue, double valueStep);

    QVariantList nodes() const;
    quint64 line0() const;
    quint64 line1() const;
    double minValue() const;
    double maxValue() const;

    //! Adds a node, on the nearest line when @p snapToLine. Returns its index, or -1 when there is
    //! no room for one there.
    Q_INVOKABLE int insertNode(double line, double value, bool snapToLine);

    //! Moves a node as far as its neighbours let it, on whole lines when @p snapToLine. The ends
    //! move only up and down.
    Q_INVOKABLE void moveNode(int index, double line, double value, bool snapToLine);

    //! Removes a node between the ends. The ends stay.
    Q_INVOKABLE bool removeNode(int index);

    //! A mouse gesture starts and ends: whatever it changed in between becomes one undo step.
    Q_INVOKABLE void beginGesture();
    Q_INVOKABLE void endGesture();

    bool canUndo() const;
    Q_INVOKABLE void undo();

    const AutomationCurve & curve() const;

    //! The keys of a node in the lists above.
    static QString lineKey();
    static QString valueKey();

    static QVariantList toVariantList(const AutomationCurve & curve);
    static AutomationCurve fromVariantList(const QVariantList & nodes);

signals:
    void nodesChanged();
    void rangeChanged();
    void historyChanged();

private:
    double constrainedValue(double value) const;

    AutomationCurve m_curve;
    quint64 m_line0 = 0;
    quint64 m_line1 = 0;
    double m_minValue = 0.0;
    double m_maxValue = 1.0;
    double m_valueStep = 0.0;

    //! The curve as it was before each gesture that changed it, the latest last.
    std::vector<AutomationCurve> m_history;
    AutomationCurve m_gestureStart;
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_EDITOR_HPP
