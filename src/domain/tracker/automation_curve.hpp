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

#ifndef AUTOMATION_CURVE_HPP
#define AUTOMATION_CURVE_HPP

#include <cstddef>
#include <optional>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! A drawn automation: nodes joined by straight lines.
//!
//! Lines are counted from wherever the owner counts them, a pattern today, so the same curve can
//! later span a song. Values are in the owner's own units, and the curve does not limit them: what
//! a value may be is the owner's business. The two end nodes are the automation's start and end
//! lines, so they move only up and down and cannot be removed; a node between them can sit
//! between lines but never on or past a neighbour.
class AutomationCurve
{
public:
    struct Node
    {
        double line = 0.0;
        double value = 0.0;

        bool operator==(const Node & other) const = default;
    };

    using NodeList = std::vector<Node>;

    //! How close two nodes may come, in lines: finer than any tick grid the song can have.
    static constexpr double minNodeGap = 1.0 / 96.0;

    AutomationCurve();
    explicit AutomationCurve(NodeList nodes);

    //! A straight line from one end to the other, the curve a ramp is drawn as.
    static AutomationCurve ramp(size_t line0, double value0, size_t line1, double value1);

    bool operator==(const AutomationCurve & other) const = default;

    const NodeList & nodes() const;
    bool isEmpty() const;

    //! The value at @p line, held at the end values outside the curve.
    double valueAt(double line) const;

    //! Adds a node between the ends. Returns its index, or nothing when @p line is not strictly
    //! between them or is too close to a node already there.
    std::optional<size_t> insert(double line, double value);

    //! Removes a node between the ends; the ends stay.
    bool remove(size_t index);

    //! Moves a node: an end only to @p value, any other also along the lines, as far as its
    //! neighbours let it.
    void move(size_t index, double line, double value);

    //! Fits the curve to new ends. Nodes keep their lines: those outside are dropped and each end
    //! gets a node at the value the curve had there, so what is left plays as before.
    void setSpan(size_t line0, size_t line1);

    //! @p line on the line grid when @p snap, otherwise as it is.
    static double snapLine(double line, bool snap);

    void serializeToXml(ProjectWriter & writer) const;
    //! Reads one node of the element the reader is on.
    void deserializeNodeFromXml(ProjectReader & reader);

private:
    NodeList m_nodes;
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_HPP
