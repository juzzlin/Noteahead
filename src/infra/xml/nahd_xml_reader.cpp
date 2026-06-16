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
#include "nahd_xml_reader.hpp"

#include <QVariant>
#include <QXmlStreamReader>

namespace noteahead {

NahdXmlReader::NahdXmlReader(QIODevice & device)
  : m_reader { std::make_unique<QXmlStreamReader>(&device) }
{
}

NahdXmlReader::NahdXmlReader(const QByteArray & data)
  : m_reader { std::make_unique<QXmlStreamReader>(data) }
{
}

NahdXmlReader::NahdXmlReader(const QString & data)
  : m_reader { std::make_unique<QXmlStreamReader>(data) }
{
}

NahdXmlReader::~NahdXmlReader() = default;

bool NahdXmlReader::readNextStartElement()
{
    return m_reader->readNextStartElement();
}

ProjectReader::TokenType NahdXmlReader::readNext()
{
    switch (m_reader->readNext()) {
    case QXmlStreamReader::StartElement:
        return TokenType::StartElement;
    case QXmlStreamReader::EndElement:
        return TokenType::EndElement;
    default:
        return TokenType::Other;
    }
}

bool NahdXmlReader::isStartElement() const
{
    return m_reader->isStartElement();
}

bool NahdXmlReader::isEndElement() const
{
    return m_reader->isEndElement();
}

void NahdXmlReader::skipCurrentElement()
{
    m_reader->skipCurrentElement();
}

QStringView NahdXmlReader::name() const
{
    return m_reader->name();
}

QVariant NahdXmlReader::attribute(const QString & name) const
{
    const auto value = m_reader->attributes().value(name);
    if (value.isNull()) {
        return {};
    }
    return value.toString();
}

bool NahdXmlReader::atEnd() const
{
    return m_reader->atEnd();
}

bool NahdXmlReader::hasError() const
{
    return m_reader->hasError();
}

QString NahdXmlReader::errorString() const
{
    return m_reader->errorString();
}

QString NahdXmlReader::readElementText()
{
    return m_reader->readElementText();
}

} // namespace noteahead
