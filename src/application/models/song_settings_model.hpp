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

#ifndef SONG_SETTINGS_MODEL_HPP
#define SONG_SETTINGS_MODEL_HPP

#include <functional>
#include <memory>

#include <QObject>
#include <QVariantList>

namespace noteahead {

class AutoNoteOffOffset;
class EditorService;

//! The current song's playback settings, for the Song Settings dialog.
//!
//! These live with the song rather than the application, so this reads and writes through the
//! EditorService's song and marks the project modified on every change.
class SongSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int autoNoteOffOffset READ autoNoteOffOffset WRITE setAutoNoteOffOffset NOTIFY changed)
    Q_PROPERTY(bool autoNoteOffSyncEnabled READ autoNoteOffSyncEnabled WRITE setAutoNoteOffSyncEnabled NOTIFY changed)
    //! The 1/N of sync mode, e.g. 16 for a sixteenth note.
    Q_PROPERTY(int autoNoteOffSyncDenominator READ autoNoteOffSyncDenominator WRITE setAutoNoteOffSyncDenominator NOTIFY changed)
    //! The denominators the dialog offers, as ints.
    Q_PROPERTY(QVariantList autoNoteOffSyncDenominators READ autoNoteOffSyncDenominators CONSTANT)

public:
    using EditorServiceS = std::shared_ptr<EditorService>;

    explicit SongSettingsModel(EditorServiceS editorService, QObject * parent = nullptr);
    ~SongSettingsModel() override;

    //! Milliseconds, used when sync is off.
    int autoNoteOffOffset() const;
    void setAutoNoteOffOffset(int milliseconds);

    bool autoNoteOffSyncEnabled() const;
    void setAutoNoteOffSyncEnabled(bool enabled);

    int autoNoteOffSyncDenominator() const;
    void setAutoNoteOffSyncDenominator(int denominator);

    QVariantList autoNoteOffSyncDenominators() const;

signals:
    //! One signal for the lot: the dialog reads all of them together and there is nothing to gain
    //! from separate notifications.
    void changed();

private:
    //! Applies a change to the current song's auto note-off offset and marks the project modified.
    void apply(const std::function<void(AutoNoteOffOffset &)> & change);

    //! The current song's offset, or the defaults a song that has none would play with.
    AutoNoteOffOffset offset() const;

    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // SONG_SETTINGS_MODEL_HPP
