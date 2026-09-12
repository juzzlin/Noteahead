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

#include "preset_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/tracker/parameter_container.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <algorithm>
#include <format>

namespace noteahead {

static const auto TAG = "PresetService";

namespace {

//! Everything a file name cannot safely carry on the file systems Noteahead runs on. The display
//! name is stored inside the file, so this only has to produce something unique and readable.
QString sanitizedFileName(const QString & presetName)
{
    QString sanitized;
    for (const auto & character : presetName) {
        sanitized.append(character.isLetterOrNumber() || character == '-' || character == '_' || character == ' ' ? character : QChar { '_' });
    }
    sanitized = sanitized.simplified();
    // A name made entirely of characters that did not survive would collide with every other such
    // name, so it falls back to a digest of what the user actually typed.
    if (sanitized.isEmpty() || sanitized == QString { sanitized.size(), QChar { '_' } }) {
        sanitized = QString::fromLatin1(QCryptographicHash::hash(presetName.toUtf8(), QCryptographicHash::Sha1).toHex().left(16));
    }
    return sanitized;
}

} // namespace

PresetService::PresetService(QString rootDirectory, QObject * parent)
  : QObject { parent }
  , m_rootDirectory { std::move(rootDirectory) }
{
    if (m_rootDirectory.isEmpty()) {
        m_rootDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
          + QDir::separator() + Constants::applicationName()
          + QDir::separator() + Constants::userPresetDirectoryName();
    }
}

QString PresetService::presetDirectory(const QString & typeId) const
{
    // Only a path: listing must not leave an empty directory behind for every device whose dialog
    // was opened. Saving is what creates it.
    return m_rootDirectory + QDir::separator() + sanitizedFileName(typeId);
}

QString PresetService::presetFilePath(const QString & typeId, const QString & presetName) const
{
    return presetDirectory(typeId) + QDir::separator() + sanitizedFileName(presetName) + Constants::presetFileExtension();
}

QString PresetService::presetNameOfFile(const QString & filePath, const QString & typeId) const
{
    QFile file { filePath };
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    NahdXmlReader reader { file };
    while (reader.readNextStartElement()) {
        if (reader.name() != Constants::NahdXml::xmlKeyPreset()) {
            reader.skipCurrentElement();
            continue;
        }
        // A preset of another type is not offered rather than refused later: the dropdown of one
        // device must never show a patch that would do nothing if picked.
        if (reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString() != typeId) {
            return {};
        }
        return reader.attribute(Constants::NahdXml::xmlKeyName()).toString();
    }
    return {};
}

QStringList PresetService::userPresetNames(const QString & typeId) const
{
    QStringList names;
    const QDir directory { presetDirectory(typeId) };
    for (const auto & fileName : directory.entryList(QStringList { "*" + Constants::presetFileExtension() }, QDir::Files)) {
        if (const auto name = presetNameOfFile(directory.filePath(fileName), typeId); !name.isEmpty()) {
            names.append(name);
        }
    }
    std::ranges::sort(names, [](const QString & lhs, const QString & rhs) {
        return lhs.compare(rhs, Qt::CaseInsensitive) < 0;
    });
    return names;
}

bool PresetService::userPresetExists(const QString & typeId, const QString & presetName) const
{
    return QFileInfo::exists(presetFilePath(typeId, presetName));
}

bool PresetService::saveUserPreset(const QString & typeId, const QString & presetName, const ParameterContainer & container)
{
    if (presetName.isEmpty()) {
        return false;
    }

    if (QDir directory { presetDirectory(typeId) }; !directory.exists() && !directory.mkpath(".")) {
        juzzlin::L(TAG).error() << std::format("Failed to create preset directory '{}'", directory.path().toStdString());
        return false;
    }

    const auto filePath = presetFilePath(typeId, presetName);
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        juzzlin::L(TAG).error() << std::format("Failed to write preset '{}'", filePath.toStdString());
        return false;
    }

    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeyPreset());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), typeId);
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), presetName);

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    container.serializeParametersToXml(writer);
    writer.writeEndElement(); // Parameters

    writer.writeEndElement(); // Preset
    writer.writeEndDocument();

    file.close();

    juzzlin::L(TAG).info() << std::format("Saved preset '{}' of type '{}'", presetName.toStdString(), typeId.toStdString());

    emit userPresetsChanged(typeId);

    return true;
}

bool PresetService::applyUserPreset(const QString & typeId, const QString & presetName, Device & device)
{
    const auto filePath = presetFilePath(typeId, presetName);
    QFile file { filePath };
    if (!file.open(QIODevice::ReadOnly)) {
        juzzlin::L(TAG).error() << std::format("Failed to read preset '{}'", filePath.toStdString());
        return false;
    }

    NahdXmlReader reader { file };
    while (reader.readNextStartElement()) {
        if (reader.name() != Constants::NahdXml::xmlKeyPreset()) {
            reader.skipCurrentElement();
            continue;
        }
        if (reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString() != typeId) {
            juzzlin::L(TAG).error() << std::format("Preset '{}' is not of type '{}'", filePath.toStdString(), typeId.toStdString());
            return false;
        }
        while (reader.readNextStartElement()) {
            if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                device.applyPresetParametersFromXml(reader);
                return true;
            }
            reader.skipCurrentElement();
        }
    }

    juzzlin::L(TAG).error() << std::format("Preset '{}' holds no parameters", filePath.toStdString());

    return false;
}

bool PresetService::deleteUserPreset(const QString & typeId, const QString & presetName)
{
    if (!QFile::remove(presetFilePath(typeId, presetName))) {
        return false;
    }

    juzzlin::L(TAG).info() << std::format("Deleted preset '{}' of type '{}'", presetName.toStdString(), typeId.toStdString());

    emit userPresetsChanged(typeId);

    return true;
}

} // namespace noteahead
