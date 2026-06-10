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
#include "common/constants.hpp"
#include "infra/data_service.hpp"

#include <QTemporaryFile>
#include <QTest>
#include <QXmlStreamWriter>

namespace noteahead {

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
    QXmlStreamWriter writer { &xml };
    writer.writeStartElement("Project");
    service.serializeDataToXml(writer, embedFiles);
    writer.writeEndElement();

    const auto expectedBase64 = fileData.toBase64();
    QVERIFY(xml.contains(QString { "path=\"%1\"" }.arg(nahdPath)));
    QVERIFY(xml.contains(expectedBase64));
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
