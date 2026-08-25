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

#include "song_overview_controller.hpp"

#include "../../application/service/song_overview_service.hpp"
#include "effect_rack_controller.hpp"

#include <QVariantMap>

namespace noteahead {

SongOverviewController::SongOverviewController(SongOverviewServiceS songOverviewService, EffectRackControllerS effectRackController, QObject * parent)
  : QObject { parent }
  , m_songOverviewService { std::move(songOverviewService) }
  , m_effectRackController { std::move(effectRackController) }
{
}

QString SongOverviewController::effectDisplayName(const QString & type, const QString & typeId) const
{
    if (!m_effectRackController) {
        return type;
    }
    // The gallery registers some entries by type string and others by id, so both have to be tried:
    // matching on one alone silently leaves half the effects showing their raw type.
    for (const auto & entry : m_effectRackController->availableEffects()) {
        const auto map = entry.toMap();
        if (const auto key = map["typeId"].toString(); key == type || key == typeId) {
            return map["name"].toString();
        }
    }
    return type;
}

void SongOverviewController::refresh()
{
    m_nodes.clear();
    m_edges.clear();

    if (!m_songOverviewService) {
        emit graphChanged();
        return;
    }

    const auto graph = m_songOverviewService->build();

    for (const auto & node : graph.nodes) {
        QVariantMap map;
        map["kind"] = static_cast<int>(node.kind);
        map["slot"] = node.slot;
        // A send node is titled by the effect standing in it, and the service can only name that by
        // its raw type -- resolving it is this layer's job, so it happens here rather than leaving
        // "earlyReflections" on the map.
        map["name"] = node.kind == SongOverviewService::NodeKind::SendEffect && !node.chain.empty()
          ? effectDisplayName(node.chain.front().effectType, node.chain.front().effectTypeId)
          : node.name;
        map["typeName"] = node.typeName;
        map["trackNames"] = node.trackNames;
        map["faderPostInserts"] = node.faderPostInserts;
        map["sendPreFader"] = node.sendPreFader;
        map["column"] = node.column;
        map["row"] = node.row;

        QVariantList chain;
        for (const auto & cell : node.chain) {
            QVariantMap cellMap;
            cellMap["kind"] = static_cast<int>(cell.kind);
            cellMap["label"] = cell.kind == SongOverviewService::CellKind::Insert
              ? effectDisplayName(cell.effectType, cell.effectTypeId)
              : QString {};
            chain.append(cellMap);
        }
        map["chain"] = chain;
        m_nodes.append(map);
    }

    for (const auto & edge : graph.edges) {
        QVariantMap map;
        map["kind"] = static_cast<int>(edge.kind);
        map["fromNode"] = edge.fromNode;
        map["toNode"] = edge.toNode;
        map["suppressed"] = edge.suppressed;
        map["preFader"] = edge.preFader;
        map["amount"] = edge.amount;
        map["tapCellIndex"] = edge.tapCellIndex;
        m_edges.append(map);
    }

    m_columnCount = graph.columnCount;
    m_rowCount = graph.rowCount;

    emit graphChanged();
}

QVariantList SongOverviewController::nodes() const
{
    return m_nodes;
}

QVariantList SongOverviewController::edges() const
{
    return m_edges;
}

int SongOverviewController::columnCount() const
{
    return m_columnCount;
}

int SongOverviewController::rowCount() const
{
    return m_rowCount;
}

void SongOverviewController::openNode(int nodeIndex)
{
    if (nodeIndex < 0 || nodeIndex >= m_nodes.size()) {
        return;
    }
    const auto node = m_nodes.at(nodeIndex).toMap();
    switch (static_cast<SongOverviewService::NodeKind>(node["kind"].toInt())) {
    case SongOverviewService::NodeKind::Device:
    case SongOverviewService::NodeKind::SubMixer:
        emit deviceOpenRequested(node["slot"].toInt());
        break;
    case SongOverviewService::NodeKind::Master:
    case SongOverviewService::NodeKind::SendEffect:
        // Both live in the master racks, which one dialog covers.
        emit masterEffectsOpenRequested();
        break;
    }
}

} // namespace noteahead
