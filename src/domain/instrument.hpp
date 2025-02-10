// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP

#include <cstdint>
#include <optional>

#include <QString>

class QXmlStreamWriter;

namespace noteahead {

class Instrument
{
public:
    explicit Instrument(QString portName);

    struct Device
    {
        Device(QString portName)
          : portName { portName }
        {
        }

        QString portName;

        uint8_t channel = 0;
    };

    Device device;

    struct Settings
    {
        std::optional<uint8_t> patch;

        struct Bank
        {
            uint8_t lsb = 0;

            uint8_t msb = 0;

            bool byteOrderSwapped = false;
        };

        std::optional<Bank> bank;

        std::optional<uint8_t> cutoff;

        std::optional<uint8_t> pan;

        std::optional<uint8_t> volume;
    };

    Settings settings;

    void serializeToXml(QXmlStreamWriter & writer) const;

    QString toString() const;

private:
    void serializeDevice(QXmlStreamWriter & writer) const;

    void serializeSettings(QXmlStreamWriter & writer) const;
};

} // namespace noteahead

#endif // INSTRUMENT_HPP
