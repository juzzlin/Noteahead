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

#include "render_settings_model.hpp"

#include "../../domain/tracker/render_settings.hpp"
#include "../../domain/tracker/song.hpp"
#include "../service/editor_service.hpp"

namespace noteahead {

RenderSettingsModel::RenderSettingsModel(EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_editorService { std::move(editorService) }
{
    if (m_editorService) {
        // A different song has different settings, so everything the dialog shows is stale.
        connect(m_editorService.get(), &EditorService::songChanged, this, &RenderSettingsModel::changed);
    }
}

RenderSettingsModel::~RenderSettingsModel() = default;

void RenderSettingsModel::apply(const std::function<void(RenderSettings &)> & change)
{
    if (!m_editorService || !m_editorService->song()) {
        return;
    }
    change(m_editorService->song()->metadata().renderSettings());
    // This is song data now, so touching it dirties the project.
    m_editorService->setIsModified(true);
    emit changed();
}

namespace {

//! Reads a value from the current song's render settings, falling back on the defaults a new song
//! would start from when there is no song at all.
template<typename GetterT>
auto read(const std::shared_ptr<EditorService> & editorService, GetterT getter)
{
    static const RenderSettings defaults;
    if (!editorService || !editorService->song()) {
        return getter(defaults);
    }
    return getter(editorService->song()->metadata().renderSettings());
}

} // namespace

int RenderSettingsModel::format() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.format(); });
}

void RenderSettingsModel::setFormat(int format)
{
    apply([&](RenderSettings & settings) { settings.setFormat(format); });
}

int RenderSettingsModel::sampleRate() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.sampleRate(); });
}

void RenderSettingsModel::setSampleRate(int sampleRate)
{
    apply([&](RenderSettings & settings) { settings.setSampleRate(sampleRate); });
}

int RenderSettingsModel::bitDepth() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.bitDepth(); });
}

void RenderSettingsModel::setBitDepth(int bitDepth)
{
    apply([&](RenderSettings & settings) { settings.setBitDepth(bitDepth); });
}

int RenderSettingsModel::oversampleFactor() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.oversampleFactor(); });
}

void RenderSettingsModel::setOversampleFactor(int factor)
{
    apply([&](RenderSettings & settings) { settings.setOversampleFactor(factor); });
}

bool RenderSettingsModel::normalizeEnabled() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.normalizeEnabled(); });
}

void RenderSettingsModel::setNormalizeEnabled(bool enabled)
{
    apply([&](RenderSettings & settings) { settings.setNormalizeEnabled(enabled); });
}

int RenderSettingsModel::normalizeLevelTenthsDb() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.normalizeLevelTenthsDb(); });
}

void RenderSettingsModel::setNormalizeLevelTenthsDb(int tenths)
{
    apply([&](RenderSettings & settings) { settings.setNormalizeLevelTenthsDb(tenths); });
}

bool RenderSettingsModel::trimEnabled() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.trimEnabled(); });
}

void RenderSettingsModel::setTrimEnabled(bool enabled)
{
    apply([&](RenderSettings & settings) { settings.setTrimEnabled(enabled); });
}

int RenderSettingsModel::trimMinutes() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.trimMinutes(); });
}

void RenderSettingsModel::setTrimMinutes(int minutes)
{
    apply([&](RenderSettings & settings) { settings.setTrimMinutes(minutes); });
}

int RenderSettingsModel::trimSeconds() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.trimSeconds(); });
}

void RenderSettingsModel::setTrimSeconds(int seconds)
{
    apply([&](RenderSettings & settings) { settings.setTrimSeconds(seconds); });
}

bool RenderSettingsModel::fadeOutEnabled() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.fadeOutEnabled(); });
}

void RenderSettingsModel::setFadeOutEnabled(bool enabled)
{
    apply([&](RenderSettings & settings) { settings.setFadeOutEnabled(enabled); });
}

int RenderSettingsModel::fadeOutSeconds() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.fadeOutSeconds(); });
}

void RenderSettingsModel::setFadeOutSeconds(int seconds)
{
    apply([&](RenderSettings & settings) { settings.setFadeOutSeconds(seconds); });
}

int RenderSettingsModel::fadeOutTenths() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.fadeOutTenths(); });
}

void RenderSettingsModel::setFadeOutTenths(int tenths)
{
    apply([&](RenderSettings & settings) { settings.setFadeOutTenths(tenths); });
}

bool RenderSettingsModel::silenceEnabled() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.silenceEnabled(); });
}

void RenderSettingsModel::setSilenceEnabled(bool enabled)
{
    apply([&](RenderSettings & settings) { settings.setSilenceEnabled(enabled); });
}

int RenderSettingsModel::silenceSeconds() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.silenceSeconds(); });
}

void RenderSettingsModel::setSilenceSeconds(int seconds)
{
    apply([&](RenderSettings & settings) { settings.setSilenceSeconds(seconds); });
}

int RenderSettingsModel::silenceTenths() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.silenceTenths(); });
}

void RenderSettingsModel::setSilenceTenths(int tenths)
{
    apply([&](RenderSettings & settings) { settings.setSilenceTenths(tenths); });
}

bool RenderSettingsModel::analyzeEnabled() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.analyzeEnabled(); });
}

void RenderSettingsModel::setAnalyzeEnabled(bool enabled)
{
    apply([&](RenderSettings & settings) { settings.setAnalyzeEnabled(enabled); });
}

bool RenderSettingsModel::fastRender() const
{
    return read(m_editorService, [](const RenderSettings & s) { return s.fastRender(); });
}

void RenderSettingsModel::setFastRender(bool fast)
{
    apply([&](RenderSettings & settings) { settings.setFastRender(fast); });
}

} // namespace noteahead
