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

#include "../common/constants.hpp"
#include "../common/xml/project_writer.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "audio/backend/sndfile_reader.hpp"
#include "xml/nahd_xml_reader.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QVariant>

#include <sndfile.h>

#include <vector>

namespace noteahead {

static const auto TAG = "DataService";

namespace {

//! Frames copied per read/write round when re-encoding, so that embedding a long sample does not
//! need the whole thing in memory at once.
constexpr int64_t transcodeChunkFrames = 8192;

QByteArray fileContents(const QString & filePath)
{
    QFile file { filePath };
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

//! The FLAC subtype that keeps everything the source subtype had, as far as FLAC can. FLAC tops out
//! at 24 bits, so 32-bit and floating-point sources land there -- the same downgrade rendering to
//! FLAC makes.
int flacSubtype(int sourceFormat)
{
    switch (sourceFormat & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
        return SF_FORMAT_PCM_S8;
    case SF_FORMAT_PCM_16:
        return SF_FORMAT_PCM_16;
    default:
        return SF_FORMAT_PCM_24;
    }
}

//! Re-encodes the audio file at \p filePath as FLAC and returns its bytes. Empty when the file is
//! not audio libsndfile can read, or holds audio FLAC cannot carry.
QByteArray flacEncoded(const QString & filePath)
{
    SndFileReader source;
    AudioFileReader::Info sourceInfo {};
    if (!source.open(filePath.toStdString(), AudioFileReader::Mode::Read, sourceInfo)) {
        return {};
    }

    // Already FLAC: the bytes it has are the bytes we want, and re-encoding would only round its
    // sample format down.
    if ((sourceInfo.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC) {
        source.close();
        return fileContents(filePath);
    }

    // libsndfile writes through a path, so the encoded copy goes via a temporary file that goes
    // when this function returns.
    QTemporaryFile temporaryFile { QDir::tempPath() + "/noteahead_embed_XXXXXX" + Constants::NahdXml::embeddedDataFileSuffix() };
    if (!temporaryFile.open()) {
        juzzlin::L(TAG).error() << "Failed to create a temporary file for re-encoding " << filePath.toStdString();
        return {};
    }
    const auto temporaryPath = temporaryFile.fileName();
    temporaryFile.close();

    SndFileReader target;
    AudioFileReader::Info targetInfo {};
    targetInfo.samplerate = sourceInfo.samplerate;
    targetInfo.channels = sourceInfo.channels;
    targetInfo.format = SF_FORMAT_FLAC | flacSubtype(sourceInfo.format);
    if (!target.open(temporaryPath.toStdString(), AudioFileReader::Mode::Write, targetInfo)) {
        juzzlin::L(TAG).error() << "FLAC cannot carry the audio of " << filePath.toStdString();
        return {};
    }

    std::vector<float> buffer(static_cast<size_t>(transcodeChunkFrames * sourceInfo.channels));
    while (const auto framesRead = source.readFloat(std::span<float> { buffer })) {
        const auto samples = static_cast<size_t>(framesRead * sourceInfo.channels);
        if (target.writeFloat(std::span<const float> { buffer.data(), samples }) != framesRead) {
            juzzlin::L(TAG).error() << "Failed to re-encode " << filePath.toStdString() << " as FLAC";
            return {};
        }
    }
    source.close();
    target.close();

    return fileContents(temporaryPath);
}

} // namespace

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
        auto data = flacEncoded(realPath);
        if (data.isEmpty()) {
            // Nothing libsndfile could re-encode. Embedding it as it stands keeps the project
            // whole; it is read back by content, so the name saying FLAC does it no harm.
            juzzlin::L(TAG).warning() << "Embedding without re-encoding: " << realPath.toStdString();
            data = fileContents(realPath);
        }
        if (data.isEmpty()) {
            juzzlin::L(TAG).error() << "Failed to open file for embedding: " << realPath.toStdString();
            continue;
        }
        juzzlin::L(TAG).info() << "Embedding file: " << realPath.toStdString() << " as " << nahdPath.toStdString();
        writer.writeStartElement(Constants::NahdXml::xmlKeyData());
        writer.writeAttribute(Constants::NahdXml::xmlKeySamplePath(), nahdPath);
        writer.writeCharacters(QString::fromLatin1(data.toBase64()));
        writer.writeEndElement();
    }
}

void DataService::clear()
{
    m_extractedFiles.clear();
    m_tempDir.reset();
}

} // namespace noteahead
