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

#ifndef INSTRUMENT_LAYER_HPP
#define INSTRUMENT_LAYER_HPP

#include "automation.hpp"

#include <cstdint>
#include <memory>

namespace noteahead {

class InstrumentLayer : public Automation
{
public:
    InstrumentLayer();

    struct Parameters
    {
        quint64 targetTrack = 0;

        uint8_t note = 0;
        bool followSourceNote = false;

        uint8_t velocity = 0;
        bool applyTargetVelocity = false;
        bool followSourceVelocity = false;
    };

    InstrumentLayer(size_t id, AutomationLocation location, Parameters parameters, QString comment, bool enabled);
    InstrumentLayer(size_t id, AutomationLocation location, Parameters parameters, QString comment);

    bool operator==(const InstrumentLayer & other) const;
    bool operator!=(const InstrumentLayer & other) const;
    bool operator<(const InstrumentLayer & other) const;

    Parameters parameters() const;
    void setParameters(const Parameters & parameters);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using InstrumentLayerU = std::unique_ptr<InstrumentLayer>;
    static InstrumentLayerU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:
    Parameters m_parameters;
};

} // namespace noteahead

#endif // INSTRUMENT_LAYER_HPP
