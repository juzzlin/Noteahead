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

#include "instrument_layers_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

static const auto TAG = "LayersModel";

InstrumentLayersModel::InstrumentLayersModel()
{
}

void InstrumentLayersModel::requestLayers()
{
    m_filter = {};

    emit layersRequested();
}

void InstrumentLayersModel::requestLayersByTrack(quint64 track)
{
    m_filter = {};
    m_filter.track = track;

    emit layersRequested();
}

void InstrumentLayersModel::requestLayersByColumn(quint64 track, quint64 column)
{
    m_filter = {};
    m_filter.track = track;
    m_filter.column = column;

    emit layersRequested();
}

InstrumentLayersModel::LayerList InstrumentLayersModel::filteredLayers(const LayerList & layers) const
{
    LayerList filtered;
    std::ranges::copy_if(layers, std::back_inserter(filtered),
                         [this](const auto & layer) {
                             if (m_filter.track.has_value() && layer.location().track() != m_filter.track.value()) {
                                 return false;
                             }
                             if (m_filter.column.has_value() && layer.location().column() != m_filter.column.value()) {
                                 return false;
                             }
                             return true;
                         });
    return filtered;
}

void InstrumentLayersModel::setLayers(LayerList layers)
{
    juzzlin::L(TAG).info() << "Setting layers: " << layers.size() << " found";
    beginResetModel();
    m_layersChanged.clear();
    m_layersDeleted.clear();
    m_layers = filteredLayers(layers);
    endResetModel();
}

int InstrumentLayersModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)

    return static_cast<int>(m_layers.size());
}

QVariant InstrumentLayersModel::data(const QModelIndex & index, int role) const
{
    if (const auto row = static_cast<size_t>(index.row()); row < m_layers.size()) {
        const auto & layer = m_layers.at(row);
        const auto & parameters = layer.parameters();

        switch (static_cast<DataRole>(role)) {
        case DataRole::Comment:
            return layer.comment();
        case DataRole::Enabled:
            return layer.enabled();
        case DataRole::Id:
            return static_cast<quint64>(layer.id());
        case DataRole::Track:
            return static_cast<quint64>(layer.location().track());
        case DataRole::Column:
            return static_cast<quint64>(layer.location().column());
        case DataRole::TargetTrack:
            return static_cast<quint64>(parameters.targetTrack);
        case DataRole::Note:
            return static_cast<int>(parameters.note);
        case DataRole::FollowSourceNote:
            return parameters.followSourceNote;
        case DataRole::Velocity:
            return static_cast<int>(parameters.velocity);
        case DataRole::ApplyTargetVelocity:
            return parameters.applyTargetVelocity;
        case DataRole::FollowSourceVelocity:
            return parameters.followSourceVelocity;
        }
    }

    return "N/A";
}

bool InstrumentLayersModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (const auto row = static_cast<size_t>(index.row()); index.isValid() && row < m_layers.size()) {
        auto layer = m_layers[row];
        bool changed = false;
        switch (static_cast<DataRole>(role)) {
        case DataRole::Comment:
            if (layer.comment() != value.toString()) {
                layer.setComment(value.toString());
                changed = true;
            }
            break;
        case DataRole::Enabled:
            if (layer.enabled() != value.toBool()) {
                layer.setEnabled(value.toBool());
                changed = true;
            }
            break;
            // These cannot be changed
        case DataRole::Column:
        case DataRole::Track:
        case DataRole::Id:
            break;
        case DataRole::TargetTrack:
            if (const auto targetTrack = value.toULongLong(); layer.parameters().targetTrack != targetTrack) {
                auto params = layer.parameters();
                params.targetTrack = targetTrack;
                layer.setParameters(params);
                changed = true;
            }
            break;
        case DataRole::Note:
            if (const auto note = value.toUInt(); layer.parameters().note != note) {
                auto params = layer.parameters();
                params.note = static_cast<uint8_t>(note);
                layer.setParameters(params);
                changed = true;
            }
            break;
        case DataRole::FollowSourceNote:
            if (const auto follow = value.toBool(); layer.parameters().followSourceNote != follow) {
                auto params = layer.parameters();
                params.followSourceNote = follow;
                layer.setParameters(params);
                changed = true;
            }
            break;
        case DataRole::Velocity:
            if (const auto velocity = value.toUInt(); layer.parameters().velocity != velocity) {
                auto params = layer.parameters();
                params.velocity = static_cast<uint8_t>(velocity);
                layer.setParameters(params);
                changed = true;
            }
            break;
        case DataRole::ApplyTargetVelocity:
            if (const auto apply = value.toBool(); layer.parameters().applyTargetVelocity != apply) {
                auto params = layer.parameters();
                params.applyTargetVelocity = apply;
                layer.setParameters(params);
                changed = true;
            }
            break;
        case DataRole::FollowSourceVelocity:
            if (const auto follow = value.toBool(); layer.parameters().followSourceVelocity != follow) {
                auto params = layer.parameters();
                params.followSourceVelocity = follow;
                layer.setParameters(params);
                changed = true;
            }
            break;
        }
        if (changed) {
            m_layers[static_cast<size_t>(index.row())] = layer;
            m_layersChanged.erase(layer);
            m_layersChanged.insert(layer);
            juzzlin::L(TAG).info() << "MIDI CC automation changed: " << layer.toString().toStdString();
            emit dataChanged(index, index, { role });
            return true;
        }
    }

    return false;
}

bool InstrumentLayersModel::removeAt(int row)
{
    return removeRows(row, 1);
}

bool InstrumentLayersModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (row < 0 || row + count > static_cast<int>(m_layers.size())) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    m_layersDeleted.insert(m_layers.at(static_cast<size_t>(row)));
    m_layers.erase(m_layers.begin() + row,
                   m_layers.begin() + row + count);
    endRemoveRows();
    return true;
}

QHash<int, QByteArray> InstrumentLayersModel::roleNames() const
{
    return {
        { static_cast<int>(DataRole::Column), "column" },
        { static_cast<int>(DataRole::Comment), "comment" },
        { static_cast<int>(DataRole::Enabled), "enabled" },
        { static_cast<int>(DataRole::Id), "id" },
        { static_cast<int>(DataRole::Track), "track" },
        { static_cast<int>(DataRole::TargetTrack), "targetTrack" },
        { static_cast<int>(DataRole::Note), "note" },
        { static_cast<int>(DataRole::FollowSourceNote), "followSourceNote" },
        { static_cast<int>(DataRole::Velocity), "velocity" },
        { static_cast<int>(DataRole::ApplyTargetVelocity), "applyTargetVelocity" },
        { static_cast<int>(DataRole::FollowSourceVelocity), "followSourceVelocity" },
    };
}

void InstrumentLayersModel::applyAll()
{
    for (auto && layer : m_layersChanged) {
        juzzlin::L(TAG).info() << "Layer automation applied: " << layer.toString().toStdString();
        emit layerChanged(layer);
    }
    m_layersChanged.clear();
    for (auto && layer : m_layersDeleted) {
        juzzlin::L(TAG).info() << "Layer automation deleted: " << layer.toString().toStdString();
        emit layerDeleted(layer);
    }
    m_layersDeleted.clear();
}

} // namespace noteahead
