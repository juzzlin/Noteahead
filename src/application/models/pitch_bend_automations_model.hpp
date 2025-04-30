// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PITCH_BEND_AUTOMATIONS_MODEL_HPP
#define PITCH_BEND_AUTOMATIONS_MODEL_HPP

#include "../../domain/pitch_bend_automation.hpp"

#include <QAbstractListModel>

#include <set>
#include <vector>

namespace noteahead {

class PitchBendAutomationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Column,
        Comment,
        Enabled,
        Id,
        Line0,
        Line1,
        Pattern,
        Track,
        Value0,
        Value1
    };

    PitchBendAutomationsModel();

    Q_INVOKABLE void requestPitchBendAutomations();
    Q_INVOKABLE void requestPitchBendAutomationsByPattern(quint64 pattern);
    Q_INVOKABLE void requestPitchBendAutomationsByTrack(quint64 pattern, quint64 track);
    Q_INVOKABLE void requestPitchBendAutomationsByColumn(quint64 pattern, quint64 track, quint64 column);
    using PitchBendAutomationList = std::vector<PitchBendAutomation>;
    void setPitchBendAutomations(PitchBendAutomationList PitchBendAutomations);

    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool removeAt(int row);
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void applyAll();

signals:
    void pitchBendAutomationChanged(const PitchBendAutomation & PitchBendAutomation);
    void pitchBendAutomationDeleted(const PitchBendAutomation & PitchBendAutomation);
    void pitchBendAutomationsRequested();

private:
    PitchBendAutomationList filteredPitchBendAutomations(const PitchBendAutomationList & PitchBendAutomations) const;

    PitchBendAutomationList m_pitchBendAutomations;
    using PitchBendAutomationSet = std::set<PitchBendAutomation>;
    PitchBendAutomationSet m_pitchBendAutomationsChanged;
    PitchBendAutomationSet m_pitchBendAutomationsDeleted;

    struct Filter
    {
        std::optional<quint64> pattern;
        std::optional<quint64> track;
        std::optional<quint64> column;
    };

    Filter m_filter;
};

} // namespace noteahead

#endif // PITCH_BEND_AUTOMATIONS_MODEL_HPP
