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

#include "song_overview_service.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "device_service.hpp"
#include "editor_service.hpp"

#include <algorithm>
#include <map>

namespace noteahead {

SongOverviewService::SongOverviewService(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
{
}

QString SongOverviewService::trackNames(int slotIndex) const
{
    if (!m_deviceService || !m_editorService) {
        return {};
    }
    if (const auto device = m_deviceService->device(static_cast<size_t>(slotIndex)); device) {
        const auto deviceName = QString::fromStdString(device->name());
        QStringList names;
        for (const auto index : m_editorService->trackIndices()) {
            if (m_editorService->instrumentPortName(index) == deviceName) {
                names << m_editorService->trackName(index);
            }
        }
        return names.join(", ");
    }
    return {};
}

std::vector<SongOverviewService::Cell> SongOverviewService::buildChain(int slotIndex) const
{
    std::vector<Cell> chain;
    const auto device = m_deviceService->device(static_cast<size_t>(slotIndex));
    if (!device) {
        return chain;
    }

    chain.push_back({ CellKind::Source, {}, {} });
    chain.push_back({ CellKind::Meter, {}, {} });

    const auto appendInserts = [&] {
        for (auto && effect : device->insertEffectRack().effects()) {
            if (effect) {
                chain.push_back({ CellKind::Insert,
                                  QString::fromStdString(effect->type()),
                                  QString::fromStdString(effect->typeId()) });
            }
        }
    };

    // The whole reason this is assembled rather than fixed. See processDeviceTask() in the engine.
    if (device->faderPosition() == Device::FaderPosition::PreInserts) {
        chain.push_back({ CellKind::Fader, {}, {} });
        appendInserts();
    } else {
        appendInserts();
        chain.push_back({ CellKind::Fader, {}, {} });
    }

    chain.push_back({ CellKind::Clip, {}, {} });
    return chain;
}

SongOverviewService::Graph SongOverviewService::build() const
{
    Graph graph;
    if (!m_deviceService) {
        return graph;
    }

    const auto slotCount = static_cast<int>(Constants::deviceRackSize());

    // Node index by device slot, so edges can be resolved once every node exists.
    std::map<int, int> nodeBySlot;

    for (int slot = 0; slot < slotCount; slot++) {
        const auto device = m_deviceService->device(static_cast<size_t>(slot));
        if (!device) {
            continue;
        }
        Node node;
        // A Sub Mixer is a device like any other; it is told apart by whether it claims members.
        node.kind = m_deviceService->subMixerMembers(slot).isEmpty() ? NodeKind::Device : NodeKind::SubMixer;
        node.slot = slot;
        node.name = QString::fromStdString(device->name());
        node.typeName = QString::fromStdString(device->typeName());
        node.trackNames = trackNames(slot);
        node.chain = buildChain(slot);
        node.faderPostInserts = device->faderPosition() == Device::FaderPosition::PostInserts;
        node.sendPreFader = device->sendTap() == Device::SendTap::PreFader;
        nodeBySlot[slot] = static_cast<int>(graph.nodes.size());
        graph.nodes.push_back(std::move(node));
    }

    // One node per populated slot of the master send rack. These sum into the master bus ahead of
    // its insert rack, which is why they are drawn feeding the master rather than leaving the mix.
    std::map<int, int> nodeBySendIndex;
    const auto & sendRack = m_deviceService->sendEffectRack();
    for (size_t sendIndex = 0; sendIndex < sendRack.effectCount(); sendIndex++) {
        if (const auto effect = sendRack.effect(sendIndex); effect) {
            Node node;
            node.kind = NodeKind::SendEffect;
            node.slot = static_cast<int>(sendIndex);
            node.name = QString::fromStdString(effect->type());
            node.chain.push_back({ CellKind::Insert,
                                   QString::fromStdString(effect->type()),
                                   QString::fromStdString(effect->typeId()) });
            nodeBySendIndex[static_cast<int>(sendIndex)] = static_cast<int>(graph.nodes.size());
            graph.nodes.push_back(std::move(node));
        }
    }

    Node master;
    master.kind = NodeKind::Master;
    master.name = tr("Master");
    for (auto && effect : m_deviceService->insertEffectRack().effects()) {
        if (effect) {
            master.chain.push_back({ CellKind::Insert,
                                     QString::fromStdString(effect->type()),
                                     QString::fromStdString(effect->typeId()) });
        }
    }
    const auto masterNode = static_cast<int>(graph.nodes.size());
    graph.nodes.push_back(std::move(master));

    // --- edges ---------------------------------------------------------------------------------
    for (const auto & [slot, nodeIndex] : nodeBySlot) {
        const auto owningSlot = m_deviceService->subMixerOwningSlot(slot);
        const auto claimed = owningSlot >= 0 && nodeBySlot.contains(owningSlot);

        Edge out;
        out.fromNode = nodeIndex;
        out.tapCellIndex = static_cast<int>(graph.nodes.at(static_cast<size_t>(nodeIndex)).chain.size());
        if (claimed) {
            out.kind = EdgeKind::SubMixerMember;
            out.toNode = nodeBySlot.at(owningSlot);
        } else {
            out.kind = EdgeKind::DirectOut;
            out.toNode = masterNode;
        }
        graph.edges.push_back(out);

        // A claimed device keeps its sends: a send bus is a parallel tap, so nothing is counted
        // twice by leaving them on, and the device keeps whatever amount it was given.
        const auto device = m_deviceService->device(static_cast<size_t>(slot));
        const auto & node = graph.nodes.at(static_cast<size_t>(nodeIndex));
        const auto faderCell = std::ranges::find_if(node.chain, [](const Cell & cell) { return cell.kind == CellKind::Fader; });
        const auto faderIndex = faderCell != node.chain.end()
          ? static_cast<int>(std::distance(node.chain.begin(), faderCell))
          : 0;

        for (size_t sendIndex = 0; sendIndex < device->reverbSendCount(); sendIndex++) {
            const auto amount = device->reverbSend(sendIndex);
            if (amount <= 0.0f || !nodeBySendIndex.contains(static_cast<int>(sendIndex))) {
                continue;
            }
            Edge send;
            send.kind = EdgeKind::Send;
            send.fromNode = nodeIndex;
            send.toNode = nodeBySendIndex.at(static_cast<int>(sendIndex));
            send.amount = amount;
            send.preFader = node.sendPreFader;
            // Captured immediately before the fader, whichever side of the inserts it sits.
            send.tapCellIndex = node.sendPreFader ? faderIndex : static_cast<int>(node.chain.size());
            graph.edges.push_back(send);
        }
    }

    for (const auto & [sendIndex, nodeIndex] : nodeBySendIndex) {
        Edge out;
        out.kind = EdgeKind::DirectOut;
        out.fromNode = nodeIndex;
        out.toNode = masterNode;
        out.tapCellIndex = 1;
        graph.edges.push_back(out);
    }

    // --- ranking -------------------------------------------------------------------------------
    // Longest path from the sources. A Sub Mixer may hold another, so this cannot be a fixed
    // three columns; iterating to a fixed point is enough for a graph the size of a device rack,
    // and the cycle check in DeviceService::canAddSubMixerMember() guarantees it terminates.
    for (int pass = 0; pass < slotCount + 2; pass++) {
        bool moved = false;
        for (const auto & edge : graph.edges) {
            if (edge.kind != EdgeKind::SubMixerMember) {
                continue;
            }
            auto & to = graph.nodes.at(static_cast<size_t>(edge.toNode));
            const auto & from = graph.nodes.at(static_cast<size_t>(edge.fromNode));
            if (to.column <= from.column) {
                to.column = from.column + 1;
                moved = true;
            }
        }
        if (!moved) {
            break;
        }
    }

    int deepestDevice = 0;
    for (const auto & node : graph.nodes) {
        if (node.kind == NodeKind::Device || node.kind == NodeKind::SubMixer) {
            deepestDevice = std::max(deepestDevice, node.column);
        }
    }
    for (auto & node : graph.nodes) {
        if (node.kind == NodeKind::SendEffect) {
            node.column = deepestDevice + 1;
        }
    }
    // One past whatever is furthest right, rather than a fixed offset: a project with no sends --
    // or no devices at all -- would otherwise be drawn with an empty column in it.
    int deepest = 0;
    for (const auto & node : graph.nodes) {
        if (node.kind != NodeKind::Master) {
            deepest = std::max(deepest, node.column + 1);
        }
    }
    graph.nodes.at(static_cast<size_t>(masterNode)).column = deepest;

    // Rows: sources first in slot order, then the send band, so that the sends read as the parallel
    // path they are rather than as another instrument.
    std::map<int, int> nextRowByColumn;
    for (auto & node : graph.nodes) {
        if (node.kind == NodeKind::SendEffect || node.kind == NodeKind::Master) {
            continue;
        }
        node.row = nextRowByColumn[node.column]++;
    }
    int sendRow = 0;
    for (const auto & [column, row] : nextRowByColumn) {
        sendRow = std::max(sendRow, row);
    }
    for (auto & node : graph.nodes) {
        if (node.kind == NodeKind::SendEffect) {
            node.row = sendRow++;
        }
    }
    graph.nodes.at(static_cast<size_t>(masterNode)).row = 0;

    for (const auto & node : graph.nodes) {
        graph.columnCount = std::max(graph.columnCount, node.column + 1);
        graph.rowCount = std::max(graph.rowCount, node.row + 1);
    }

    return graph;
}

} // namespace noteahead
