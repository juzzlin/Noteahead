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

#ifndef PROJECT_READER_HPP
#define PROJECT_READER_HPP

class QString;
class QStringView;
class QVariant;

namespace noteahead {

class ProjectReader
{
public:
    enum class TokenType
    {
        StartElement,
        EndElement,
        Other
    };

    virtual ~ProjectReader() = default;

    virtual bool readNextStartElement() = 0;
    virtual TokenType readNext() = 0;
    virtual bool isStartElement() const = 0;
    virtual bool isEndElement() const = 0;
    virtual void skipCurrentElement() = 0;

    virtual QStringView name() const = 0;
    virtual QVariant attribute(const QString & name) const = 0;

    virtual bool atEnd() const = 0;
    virtual bool hasError() const = 0;
    virtual QString errorString() const = 0;

    virtual QString readElementText() = 0;
};

} // namespace noteahead

#endif // PROJECT_READER_HPP
