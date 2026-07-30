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

#include "song_settings_model.hpp"

#include "../../domain/tracker/auto_note_off_offset.hpp"
#include "../../domain/tracker/song.hpp"
#include "../service/editor_service.hpp"

namespace noteahead {

SongSettingsModel::SongSettingsModel(EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_editorService { std::move(editorService) }
{
    if (m_editorService) {
        // A different song has different settings, so everything the dialog shows is stale.
        connect(m_editorService.get(), &EditorService::songChanged, this, &SongSettingsModel::changed);
    }
}

SongSettingsModel::~SongSettingsModel() = default;

AutoNoteOffOffset SongSettingsModel::offset() const
{
    if (!m_editorService || !m_editorService->song()) {
        return {};
    }
    return m_editorService->song()->settings().autoNoteOffOffset().value_or(AutoNoteOffOffset {});
}

void SongSettingsModel::apply(const std::function<void(AutoNoteOffOffset &)> & change)
{
    if (!m_editorService || !m_editorService->song()) {
        return;
    }
    auto offset = this->offset();
    change(offset);
    m_editorService->song()->settings().setAutoNoteOffOffset(offset);
    // This is song data now, so touching it dirties the project.
    m_editorService->setIsModified(true);
    emit changed();
}

int SongSettingsModel::autoNoteOffOffset() const
{
    return static_cast<int>(offset().milliseconds().count());
}

void SongSettingsModel::setAutoNoteOffOffset(int milliseconds)
{
    apply([&](AutoNoteOffOffset & offset) { offset.setMilliseconds(std::chrono::milliseconds { milliseconds }); });
}

bool SongSettingsModel::autoNoteOffSyncEnabled() const
{
    return offset().syncEnabled();
}

void SongSettingsModel::setAutoNoteOffSyncEnabled(bool enabled)
{
    apply([&](AutoNoteOffOffset & offset) { offset.setSyncEnabled(enabled); });
}

int SongSettingsModel::autoNoteOffSyncDenominator() const
{
    return offset().syncDenominator();
}

void SongSettingsModel::setAutoNoteOffSyncDenominator(int denominator)
{
    apply([&](AutoNoteOffOffset & offset) { offset.setSyncDenominator(denominator); });
}

QVariantList SongSettingsModel::autoNoteOffSyncDenominators() const
{
    QVariantList denominators;
    for (auto && denominator : AutoNoteOffOffset::syncDenominators()) {
        denominators.append(denominator);
    }
    return denominators;
}

} // namespace noteahead
