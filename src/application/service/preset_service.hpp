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

#ifndef PRESET_SERVICE_HPP
#define PRESET_SERVICE_HPP

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace noteahead {

class Device;
class Effect;
class ParameterContainer;
class ProjectReader;

//! The user's own presets, stored as one file per preset under the configuration directory.
//!
//! A preset is what a factory preset is: a name and a set of parameter values. It carries nothing
//! else -- no port, no channel, no insert effects -- so that a preset saved on one device can never
//! drag anything of that device along into another.
//!
//! The service knows only a type id and a ParameterContainer, so devices and effects are the same
//! thing to it. Presets of different types live in different subdirectories, and a file naming a
//! type other than the one being asked for is ignored rather than offered.
class PresetService : public QObject
{
    Q_OBJECT

public:
    //! @p rootDirectory is where the per-type preset directories live. Empty means the standard
    //! configuration directory, which is what the application uses; a test passes its own.
    explicit PresetService(QString rootDirectory = {}, QObject * parent = nullptr);

    //! Directory holding the presets of @p typeId. Created on the first save into it, not by
    //! merely being asked for -- a device whose dialog was opened and nothing saved leaves nothing.
    QString presetDirectory(const QString & typeId) const;

    //! Display names of the presets stored for @p typeId, sorted case-insensitively.
    QStringList userPresetNames(const QString & typeId) const;

    bool userPresetExists(const QString & typeId, const QString & presetName) const;

    //! Writes @p container's authored values under @p presetName, replacing a preset of the same
    //! name. Values are stored in XML units with their ranges, so a preset keeps meaning what it
    //! meant even after a parameter is renamed or its range revised.
    bool saveUserPreset(const QString & typeId, const QString & presetName, const ParameterContainer & container);

    //! Applies the preset @p presetName to @p device. Everything the preset does not name goes
    //! back to its default, exactly as a factory preset does.
    bool applyUserPreset(const QString & typeId, const QString & presetName, Device & device);

    //! The same for an effect. Two overloads rather than one taking a common base: the reset a
    //! preset begins with means different things to the two, and neither shares an interface with
    //! the other beyond the parameters themselves.
    bool applyUserPreset(const QString & typeId, const QString & presetName, Effect & effect);

    bool deleteUserPreset(const QString & typeId, const QString & presetName);

signals:
    //! The presets of @p typeId have changed on disk.
    void userPresetsChanged(QString typeId);

private:
    //! Path of the file holding @p presetName. The name is sanitized into something every file
    //! system accepts; the name the user typed is kept inside the file, so nothing is lost to this.
    QString presetFilePath(const QString & typeId, const QString & presetName) const;

    //! The display name stored in @p filePath, or empty when the file is not a preset of @p typeId.
    QString presetNameOfFile(const QString & filePath, const QString & typeId) const;

    //! Opens the preset, walks to its <Parameters> element and hands the reader to @p apply.
    using PresetApplier = std::function<void(ProjectReader &)>;
    bool readUserPreset(const QString & typeId, const QString & presetName, const PresetApplier & apply);

    QString m_rootDirectory;
};

} // namespace noteahead

#endif // PRESET_SERVICE_HPP
