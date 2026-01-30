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

#include "data_service_test.hpp"
#include "../../common/constants.hpp"
#include "../../infra/audio/backend/sndfile_reader.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QTest>

#include <sndfile.h>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

//! A ramp with a little detail on it, so that it both compresses and is easy to compare.
std::vector<float> testFrames(size_t frameCount)
{
    std::vector<float> frames;
    frames.reserve(frameCount);
    for (size_t i = 0; i < frameCount; i++) {
        frames.push_back(static_cast<float>(std::sin(static_cast<double>(i) * 0.01) * 0.5));
    }
    return frames;
}

//! Writes \p frames as a mono file of the given libsndfile format and returns its path.
QString writeAudioFile(const QString & fileName, int format, const std::vector<float> & frames)
{
    const auto filePath = QDir { QDir::tempPath() }.absoluteFilePath(fileName);
    QFile::remove(filePath);

    SndFileReader writer;
    AudioFileReader::Info info {};
    info.samplerate = 44100;
    info.channels = 1;
    info.format = format;
    if (!writer.open(filePath.toStdString(), AudioFileReader::Mode::Write, info)) {
        return {};
    }
    writer.writeFloat(frames);
    writer.close();
    return filePath;
}

//! The decoded contents of the single <Data> element of \p xml.
QByteArray embeddedData(DataService & service, const QString & xml, const QString & nahdPath)
{
    service.extractDataFromXml(xml);
    const auto resolvedPath = service.resolvePath(nahdPath);
    QFile file { resolvedPath };
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray {};
}

QString embedToXml(DataService & service, const QString & nahdPath, const QString & realPath)
{
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement("Project");
    service.serializeDataToXml(writer, { { nahdPath, realPath } });
    writer.writeEndElement();
    return xml;
}

} // namespace

void DataServiceTest::test_extractAndResolve_shouldExtractFilesFromXml()
{
    DataService service;
    const auto fileName = "test.wav";
    const auto nahdPath = "nahd://" + QString { fileName };
    const auto fileData = QByteArray { "dummy wave data" };
    const auto base64Data = fileData.toBase64();

    const auto xml = QString { R"(
        <Project>
            <Data path="%1">%2</Data>
        </Project>
    )" }
                       .arg(nahdPath, QString::fromUtf8(base64Data));

    service.extractDataFromXml(xml);

    const auto resolvedPath = service.resolvePath(nahdPath);
    QVERIFY(resolvedPath != nahdPath);
    QVERIFY(QFile::exists(resolvedPath));

    QFile extractedFile { resolvedPath };
    QVERIFY(extractedFile.open(QIODevice::ReadOnly));
    QCOMPARE(extractedFile.readAll(), fileData);
}

void DataServiceTest::test_serializeDataToXml_shouldEmbedFilesAsBase64()
{
    // Not audio libsndfile can read, so it goes in as it stands rather than being lost.
    DataService service;
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileData = QByteArray { "some more dummy data" };
    tempFile.write(fileData);
    tempFile.close();

    const auto nahdPath = "nahd://embedded.wav";
    std::map<QString, QString> embedFiles;
    embedFiles[nahdPath] = tempFile.fileName();

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement("Project");
    service.serializeDataToXml(writer, embedFiles);
    writer.writeEndElement();

    const auto expectedBase64 = fileData.toBase64();
    QVERIFY(xml.contains(QString { "path=\"%1\"" }.arg(nahdPath)));
    QVERIFY(xml.contains(expectedBase64));
}

void DataServiceTest::test_serializeDataToXml_wav_shouldEmbedItAsFlac()
{
    // A project carries the audio of its pads, not the file they came in, so a WAV is re-encoded on
    // the way in: FLAC loses nothing and takes roughly half the room.
    const auto frames = testFrames(20000);
    const auto sourcePath = writeAudioFile("noteahead_data_service_test.wav", SF_FORMAT_WAV | SF_FORMAT_PCM_24, frames);
    QVERIFY(!sourcePath.isEmpty());

    DataService service;
    const auto nahdPath = Constants::NahdXml::embeddedDataPathPrefix() + "embedded" + Constants::NahdXml::embeddedDataFileSuffix();
    const auto data = embeddedData(service, embedToXml(service, nahdPath, sourcePath), nahdPath);

    QCOMPARE(data.left(4), QByteArray { "fLaC" });
    QVERIFY2(data.size() < QFileInfo { sourcePath }.size(), qPrintable(QString::number(data.size())));

    // The audio survives the re-encoding sample for sample.
    SndFileReader reader;
    AudioFileReader::Info info {};
    QVERIFY(reader.open(service.resolvePath(nahdPath).toStdString(), AudioFileReader::Mode::Read, info));
    QCOMPARE(info.format & SF_FORMAT_TYPEMASK, SF_FORMAT_FLAC);
    QCOMPARE(info.channels, 1);
    QCOMPARE(info.samplerate, 44100);
    QCOMPARE(static_cast<size_t>(info.frames), frames.size());

    std::vector<float> readBack(frames.size());
    reader.readFloat(readBack);
    reader.close();
    for (size_t i = 0; i < frames.size(); i++) {
        QVERIFY2(std::abs(readBack.at(i) - frames.at(i)) < 1e-5f, qPrintable(QString::number(i)));
    }

    QFile::remove(sourcePath);
}

void DataServiceTest::test_serializeDataToXml_flac_shouldEmbedItVerbatim()
{
    // Already in the format it would be re-encoded into: re-encoding it could only round it down.
    const auto sourcePath = writeAudioFile("noteahead_data_service_test.flac", SF_FORMAT_FLAC | SF_FORMAT_PCM_24, testFrames(5000));
    QVERIFY(!sourcePath.isEmpty());

    QFile sourceFile { sourcePath };
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));
    const auto sourceData = sourceFile.readAll();
    sourceFile.close();

    DataService service;
    const auto nahdPath = Constants::NahdXml::embeddedDataPathPrefix() + "embedded" + Constants::NahdXml::embeddedDataFileSuffix();
    QCOMPARE(embeddedData(service, embedToXml(service, nahdPath, sourcePath), nahdPath), sourceData);

    QFile::remove(sourcePath);
}

void DataServiceTest::test_clear_shouldRemoveTempDirAndExtractedFiles()
{
    DataService service;
    const auto xml = QString { R"(<Project><Data path="nahd://test.wav">ZHVtbXk=</Data></Project>)" };
    service.extractDataFromXml(xml);

    const auto resolvedPath = service.resolvePath("nahd://test.wav");
    QVERIFY(QFile::exists(resolvedPath));

    service.clear();

    QVERIFY(!QFile::exists(resolvedPath));
    QCOMPARE(service.resolvePath("nahd://test.wav"), QString { "nahd://test.wav" });
}

void DataServiceTest::test_resolvePath_shouldReturnOriginalPath_whenNotFound()
{
    DataService service;
    const auto originalPath = "some/other/path.wav";
    QCOMPARE(service.resolvePath(originalPath), QString { originalPath });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DataServiceTest)
