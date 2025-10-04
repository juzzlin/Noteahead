// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#include "settings_service.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/settings.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace noteahead {

static const auto TAG = "SettingsService";

SettingsService::SettingsService()
  : m_controllerPort { Settings::controllerPort("") }
  , m_visibleLines { Settings::visibleLines(32) }
{
}

int SettingsService::autoNoteOffOffset() const
{
    return Settings::autoNoteOffOffset(250);
}

void SettingsService::setAutoNoteOffOffset(int autoNoteOffOffset)
{
    Settings::setAutoNoteOffOffset(autoNoteOffOffset);
}

QString SettingsService::controllerPort() const
{
    return m_controllerPort;
}

void SettingsService::setControllerPort(QString controllerPort)
{
    if (m_controllerPort != controllerPort) {
        juzzlin::L(TAG).info() << "Setting controller port: " << controllerPort.toStdString();
        m_controllerPort = controllerPort;
        Settings::setControllerPort(controllerPort);
        emit controllerPortChanged();
    }
}

QSize SettingsService::windowSize(QSize defaultSize) const
{
    return Settings::windowSize(defaultSize);
}

void SettingsService::setWindowSize(QSize size)
{
    Settings::setWindowSize(size);
}

int SettingsService::step(int defaultStep) const
{
    return Settings::step(defaultStep);
}

void SettingsService::setStep(int step)
{
    Settings::setStep(step);
}

int SettingsService::velocity(int defaultVelocity) const
{
    return Settings::velocity(defaultVelocity);
}

void SettingsService::setVelocity(int velocity)
{
    Settings::setVelocity(velocity);
}

int SettingsService::visibleLines() const
{
    return m_visibleLines;
}

void SettingsService::setVisibleLines(int visibleLines)
{
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        Settings::setVisibleLines(visibleLines);
        emit visibleLinesChanged();
    }
}

bool SettingsService::recordingEnabled() const
{
    return Settings::recordingEnabled();
}

void SettingsService::setRecordingEnabled(bool enabled)
{
    return Settings::setRecordingEnabled(enabled);
}

int SettingsService::audioBufferSize() const
{
    return Settings::audioBufferSize();
}

void SettingsService::setAudioBufferSize(int samples)
{
    Settings::setAudioBufferSize(samples);
}

SettingsService::~SettingsService() = default;

} // namespace noteahead
