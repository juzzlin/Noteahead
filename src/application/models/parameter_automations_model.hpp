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

#ifndef PARAMETER_AUTOMATIONS_MODEL_HPP
#define PARAMETER_AUTOMATIONS_MODEL_HPP

#include "../../domain/tracker/parameter_automation.hpp"

#include <QAbstractListModel>
#include <QVariantMap>

#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace noteahead {

//! The parameter automations the edit dialog lists, filtered the way the MIDI CC and pitch bend
//! dialogs filter theirs. Edits and deletions are held here until applyAll(), so that Cancel leaves
//! the song alone.
class ParameterAutomationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Column,
        Comment,
        Curve,
        EffectId,
        Enabled,
        EventsPerBeat,
        Id,
        Line0,
        Line1,
        LineOffset,
        Mode,
        Modulation_Amplitude,
        Modulation_Cycles,
        Modulation_Inverted,
        Modulation_Offset,
        Modulation_Type,
        Parameter,
        Pattern,
        Track,
        Value0,
        Value1
    };

    ParameterAutomationsModel();

    Q_INVOKABLE void requestParameterAutomations();
    Q_INVOKABLE void requestParameterAutomationsByPattern(quint64 pattern);
    Q_INVOKABLE void requestParameterAutomationsByTrack(quint64 pattern, quint64 track);
    Q_INVOKABLE void requestParameterAutomationsByColumn(quint64 pattern, quint64 track, quint64 column);
    Q_INVOKABLE void requestParameterAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line);
    //! Every automation on the selected columns that runs over any of the selected lines.
    Q_INVOKABLE void requestParameterAutomationsBySelection(quint64 pattern, quint64 track, quint64 minColumn, quint64 maxColumn, quint64 minLine, quint64 maxLine);

    using ParameterAutomationList = std::vector<ParameterAutomation>;
    void setParameterAutomations(ParameterAutomationList parameterAutomations);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool removeAt(int row);
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;

    //! Hands every held edit and deletion on to be written into the song.
    Q_INVOKABLE void applyAll();

    //! Overwrites the automation at the given row with the settings of another one.
    //!
    //! Everything but the location, the enabled flag and the target: a copy changes what an
    //! automation does, not where it lives, whether it is on, or what it drives. Keys missing from
    //! the map are left alone.
    Q_INVOKABLE void applyValues(int index, const QVariantMap & values);

signals:
    void parameterAutomationChanged(const ParameterAutomation & parameterAutomation);
    void parameterAutomationDeleted(const ParameterAutomation & parameterAutomation);
    void parameterAutomationsRequested();
    //! One row was replaced wholesale by applyValues(). The row's spin boxes stop following the
    //! model once typed into, so they need telling.
    void automationReplaced(int index);

private:
    ParameterAutomationList filtered(const ParameterAutomationList & parameterAutomations) const;

    ParameterAutomationList m_parameterAutomations;
    using ParameterAutomationSet = std::set<ParameterAutomation>;
    ParameterAutomationSet m_changed;
    ParameterAutomationSet m_deleted;

    //! Inclusive at both ends. One column or one line is a range that starts where it ends.
    using Range = std::pair<quint64, quint64>;

    struct Filter
    {
        std::optional<quint64> pattern;
        std::optional<quint64> track;
        std::optional<Range> columns;
        std::optional<Range> lines;
    };

    Filter m_filter;
};

} // namespace noteahead

#endif // PARAMETER_AUTOMATIONS_MODEL_HPP
