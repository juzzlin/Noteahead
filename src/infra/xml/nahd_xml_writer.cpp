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
#include "nahd_xml_writer.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

NahdXmlWriter::NahdXmlWriter(QIODevice & device)
  : m_writer { std::make_unique<QXmlStreamWriter>(&device) }
{
}

NahdXmlWriter::NahdXmlWriter(QByteArray & array)
  : m_writer { std::make_unique<QXmlStreamWriter>(&array) }
{
}

NahdXmlWriter::NahdXmlWriter(QString & string)
  : m_writer { std::make_unique<QXmlStreamWriter>(&string) }
{
}

NahdXmlWriter::~NahdXmlWriter() = default;

void NahdXmlWriter::writeStartDocument()
{
    m_writer->writeStartDocument();
}

void NahdXmlWriter::writeEndDocument()
{
    m_writer->writeEndDocument();
}

void NahdXmlWriter::writeStartElement(const QString & name)
{
    m_writer->writeStartElement(name);
}

void NahdXmlWriter::writeEndElement()
{
    m_writer->writeEndElement();
}

void NahdXmlWriter::writeAttribute(const QString & name, const QString & value)
{
    m_writer->writeAttribute(name, value);
}

void NahdXmlWriter::writeCharacters(const QString & text)
{
    m_writer->writeCharacters(text);
}

void NahdXmlWriter::setAutoFormatting(bool enable)
{
    m_writer->setAutoFormatting(enable);
}

void NahdXmlWriter::setAutoFormattingIndent(int indent)
{
    m_writer->setAutoFormattingIndent(indent);
}

} // namespace noteahead
