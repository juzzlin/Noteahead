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

#include "data_service.hpp"

#include "common/constants.hpp"
#include "common/xml/project_writer.hpp"
#include "contrib/SimpleLogger/src/simple_logger.hpp"
#include "infra/xml/nahd_xml_reader.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>

namespace noteahead {

static const auto TAG = "DataService";

DataService::DataService() = default;

DataService::~DataService() = default;

void DataService::extractDataFromXml(const QString & xml)
{
    juzzlin::L(TAG).info() << "Extracting embedded data from XML";

    clear();

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (!m_tempDir->isValid()) {
        juzzlin::L(TAG).error() << "Failed to create temporary directory for embedded data";
        return;
    }

    juzzlin::L(TAG).info() << "Temporary directory created: " << m_tempDir->path().toStdString();

    NahdXmlReader reader { xml };
    while (!reader.atEnd()) {
        if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeyData()) {
            const auto nahdPath = reader.attribute(Constants::NahdXml::xmlKeySamplePath()).toString();
            const auto base64Data = reader.readElementText().toUtf8();
            const auto decodedData = QByteArray::fromBase64(base64Data);

            if (nahdPath.isEmpty()) {
                juzzlin::L(TAG).warning() << "Found <Data> element with missing path attribute";
            } else {
                const auto fileName = QFileInfo { nahdPath }.fileName();
                const auto tempFilePath = m_tempDir->filePath(fileName);

                QFile file { tempFilePath };
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(decodedData);
                    file.close();
                    m_extractedFiles[nahdPath] = tempFilePath;
                    juzzlin::L(TAG).info() << "Extracted: " << nahdPath.toStdString() << " -> " << tempFilePath.toStdString();
                } else {
                    juzzlin::L(TAG).error() << "Failed to write extracted file: " << tempFilePath.toStdString();
                }
            }
        }
        reader.readNext();
    }

    if (reader.hasError()) {
        juzzlin::L(TAG).error() << "XML error during data extraction: " << reader.errorString().toStdString();
    }
}

void DataService::extractData(ProjectReader & reader)
{
    if (!m_tempDir) {
        m_tempDir = std::make_unique<QTemporaryDir>();
        if (!m_tempDir->isValid()) {
            juzzlin::L(TAG).error() << "Failed to create temporary directory for embedded data";
            return;
        }
        juzzlin::L(TAG).info() << "Temporary directory created: " << m_tempDir->path().toStdString();
    }

    if (reader.isStartElement() && reader.name() == Constants::NahdXml::xmlKeyData()) {
        const auto nahdPath = reader.attribute(Constants::NahdXml::xmlKeySamplePath()).toString();
        const auto base64Data = reader.readElementText().toUtf8();
        const auto decodedData = QByteArray::fromBase64(base64Data);

        if (nahdPath.isEmpty()) {
            juzzlin::L(TAG).warning() << "Found <Data> element with missing path attribute";
        } else {
            const auto fileName = QFileInfo { nahdPath }.fileName();
            const auto tempFilePath = m_tempDir->filePath(fileName);

            QFile file { tempFilePath };
            if (file.open(QIODevice::WriteOnly)) {
                file.write(decodedData);
                file.close();
                m_extractedFiles[nahdPath] = tempFilePath;
                juzzlin::L(TAG).info() << "Extracted: " << nahdPath.toStdString() << " -> " << tempFilePath.toStdString();
            } else {
                juzzlin::L(TAG).error() << "Failed to write extracted file: " << tempFilePath.toStdString();
            }
        }
    }
}

QString DataService::resolvePath(const QString & nahdPath) const
{
    if (const auto it = m_extractedFiles.find(nahdPath); it != m_extractedFiles.end()) {
        return it->second;
    }
    return nahdPath;
}

void DataService::serializeDataToXml(ProjectWriter & writer, const std::map<QString, QString> & embedFiles) const
{
    for (const auto & [nahdPath, realPath] : embedFiles) {
        QFile file { realPath };
        if (file.open(QIODevice::ReadOnly)) {
            juzzlin::L(TAG).info() << "Embedding file: " << realPath.toStdString() << " as " << nahdPath.toStdString();
            writer.writeStartElement(Constants::NahdXml::xmlKeyData());
            writer.writeAttribute(Constants::NahdXml::xmlKeySamplePath(), nahdPath);
            writer.writeCharacters(QString::fromLatin1(file.readAll().toBase64()));
            writer.writeEndElement();
        } else {
            juzzlin::L(TAG).error() << "Failed to open file for embedding: " << realPath.toStdString();
        }
    }
}

void DataService::clear()
{
    m_extractedFiles.clear();
    m_tempDir.reset();
}

} // namespace noteahead
