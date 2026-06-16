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

#ifndef NAHD_XML_READER_HPP
#define NAHD_XML_READER_HPP

#include "../../common/xml/project_reader.hpp"

#include <memory>

class QByteArray;
class QIODevice;
class QXmlStreamReader;

namespace noteahead {

class NahdXmlReader : public ProjectReader
{
public:
    explicit NahdXmlReader(QIODevice & device);
    explicit NahdXmlReader(const QByteArray & data);
    explicit NahdXmlReader(const QString & data);

    ~NahdXmlReader() override;

    bool readNextStartElement() override;
    TokenType readNext() override;
    bool isStartElement() const override;
    bool isEndElement() const override;
    void skipCurrentElement() override;

    QStringView name() const override;
    QVariant attribute(const QString & name) const override;

    bool atEnd() const override;
    bool hasError() const override;
    QString errorString() const override;

    QString readElementText() override;

private:
    std::unique_ptr<QXmlStreamReader> m_reader;
};

} // namespace noteahead

#endif // NAHD_XML_READER_HPP
