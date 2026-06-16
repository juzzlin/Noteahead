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

#ifndef PROJECT_WRITER_HPP
#define PROJECT_WRITER_HPP

class QString;

namespace noteahead {

class ProjectWriter
{
public:
    virtual ~ProjectWriter() = default;

    virtual void writeStartDocument() = 0;
    virtual void writeEndDocument() = 0;

    virtual void writeStartElement(const QString & name) = 0;
    virtual void writeEndElement() = 0;

    virtual void writeAttribute(const QString & name, const QString & value) = 0;
    virtual void writeCharacters(const QString & text) = 0;

    virtual void setAutoFormatting(bool enable) = 0;
    virtual void setAutoFormattingIndent(int indent) = 0;
};

} // namespace noteahead

#endif // PROJECT_WRITER_HPP
