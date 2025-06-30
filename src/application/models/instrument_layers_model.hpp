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

#ifndef INSTRUMENT_LAYERS_MODEL_HPP
#define INSTRUMENT_LAYERS_MODEL_HPP

#include "../../domain/instrument_layer.hpp"

#include <QAbstractListModel>

#include <set>
#include <vector>

namespace noteahead {

class InstrumentLayersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Column,
        Comment,
        Enabled,
        Id,
        Track,
        TargetTrack,
        Note,
        FollowSourceNote,
        Velocity,
        ApplyTargetVelocity,
        FollowSourceVelocity
    };

    InstrumentLayersModel();

    Q_INVOKABLE void requestLayers();
    Q_INVOKABLE void requestLayersByTrack(quint64 track);
    Q_INVOKABLE void requestLayersByColumn(quint64 track, quint64 column);
    using LayerList = std::vector<InstrumentLayer>;
    void setLayers(LayerList layers);

    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool removeAt(int row);
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void applyAll();

signals:
    void layerChanged(const InstrumentLayer & layer);
    void layerDeleted(const InstrumentLayer & layer);
    void layersRequested();

private:
    LayerList filteredLayers(const LayerList & midiCcAutomations) const;

    LayerList m_layers;
    using LayerSet = std::set<InstrumentLayer>;
    LayerSet m_layersChanged;
    LayerSet m_layersDeleted;

    struct Filter
    {
        std::optional<quint64> pattern;
        std::optional<quint64> track;
        std::optional<quint64> column;
        std::optional<quint64> line;
    };

    Filter m_filter;
};

} // namespace noteahead

#endif // INSTRUMENT_LAYERS_MODEL_HPP
