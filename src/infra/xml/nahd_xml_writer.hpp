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

#ifndef NAHD_XML_WRITER_HPP
#define NAHD_XML_WRITER_HPP

#include "common/xml/project_writer.hpp"

#include <memory>

class QByteArray;
class QIODevice;
class QXmlStreamWriter;

namespace noteahead {

class NahdXmlWriter : public ProjectWriter
{
public:
    explicit NahdXmlWriter(QIODevice & device);
    explicit NahdXmlWriter(QByteArray & array);
    explicit NahdXmlWriter(QString & string);

    ~NahdXmlWriter() override;

    void writeStartDocument() override;
    void writeEndDocument() override;

    void writeStartElement(const QString & name) override;
    void writeEndElement() override;

    void writeAttribute(const QString & name, const QString & value) override;
    void writeCharacters(const QString & text) override;

    void setAutoFormatting(bool enable) override;
    void setAutoFormattingIndent(int indent) override;

private:
    std::unique_ptr<QXmlStreamWriter> m_writer;
};

} // namespace noteahead

#endif // NAHD_XML_WRITER_HPP
