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

#ifndef SONG_OVERVIEW_SERVICE_HPP
#define SONG_OVERVIEW_SERVICE_HPP

#include <QObject>
#include <QString>

#include <memory>
#include <vector>

namespace noteahead {

class DeviceService;
class EditorService;

//! Builds the song's signal flow as a laid-out graph, for the Song Overview to draw.
//!
//! Deliberately produces grid coordinates -- column and row indices -- rather than pixels, so that
//! the shape of the graph can be tested without a window and the renderer is free to size cells
//! however it likes.
//!
//! The chain of each device is assembled in the order the engine actually runs it, which is not
//! fixed: Device::faderPosition() decides whether the fader comes before or after the insert rack.
//! A map that drew one order for everything would misreport half the settings it exists to show.
class SongOverviewService : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    SongOverviewService(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);

    enum class NodeKind
    {
        Device,
        SubMixer,
        Master,
        SendEffect
    };

    enum class CellKind
    {
        //! The device itself: what processAudio() produces, gain and pan included.
        Source,
        //! Level tap, post-gain and pre-insert. Fixed: it does not move with the fader.
        Meter,
        Fader,
        Insert,
        //! Clip and scope taps, both on the final output.
        Clip
    };

    //! One step of a device's chain, left to right in the order the engine runs it.
    struct Cell
    {
        CellKind kind {};
        //! Raw effect type and id for an Insert, empty otherwise. Resolving these to the names the
        //! gallery shows is the view's job: the list that maps them lives in a controller, and a
        //! service must not reach up into one.
        QString effectType;
        QString effectTypeId;
    };

    enum class EdgeKind
    {
        //! Straight to the master bus.
        DirectOut,
        //! Into the Sub Mixer that claimed this device.
        SubMixerMember,
        Send
    };

    struct Node
    {
        NodeKind kind {};
        //! Device slot, or the send rack index for a SendEffect. -1 for the master.
        int slot { -1 };
        QString name;
        QString typeName;
        //! Tracks that play this device, comma separated. Empty for anything but a device.
        QString trackNames;
        std::vector<Cell> chain;
        bool faderPostInserts { false };
        bool sendPreFader { false };
        int column { 0 };
        int row { 0 };
    };

    struct Edge
    {
        EdgeKind kind {};
        //! Indices into nodes().
        int fromNode { -1 };
        int toNode { -1 };
        //! A device a Sub Mixer has claimed still reaches the master, but only through the Sub
        //! Mixer: its own direct path is switched off. Drawn, but drawn as not carrying anything.
        bool suppressed { false };
        bool preFader { false };
        float amount { 0.0f };
        //! Where along the source's chain the edge leaves, as an index into it.
        //!
        //! A pre-fader send is captured immediately before the fader whichever side of the insert
        //! rack the fader sits, so this is the fader's own index. What differs between the two
        //! fader positions is what has already happened by then -- with the fader after the
        //! inserts, a "pre-fader" send has been through them.
        int tapCellIndex { 0 };
    };

    struct Graph
    {
        std::vector<Node> nodes;
        std::vector<Edge> edges;
        int columnCount { 0 };
        int rowCount { 0 };
    };

    //! Reads the current devices and returns the graph. Cheap enough to call whenever the rack
    //! changes; nothing is cached, so it cannot go stale.
    Graph build() const;

    //! Tracks that play the device in the given slot, comma separated.
    QString trackNames(int slotIndex) const;

private:
    std::vector<Cell> buildChain(int slotIndex) const;

    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // SONG_OVERVIEW_SERVICE_HPP
