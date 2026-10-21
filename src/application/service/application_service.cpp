// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "application_service.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/tracker/column_settings.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/song.hpp"
#include "../note_converter.hpp"
#include "../state_machine.hpp"
#include "editor_service.hpp"
#include "player_service.hpp"
#include "random_service.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "recent_files_manager.hpp"
#include <QDir>
#include <QGuiApplication>
#include <QQuickWindow>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace noteahead {

static const auto TAG = "ApplicationService";

using namespace std::chrono_literals;

ApplicationService::ApplicationService()
{
}

QString ApplicationService::applicationName() const
{
    return Constants::applicationName();
}

QString ApplicationService::applicationVersion() const
{
    return Constants::applicationVersion();
}

QString ApplicationService::copyright() const
{
    return Constants::copyright();
}

QString ApplicationService::license() const
{
    return Constants::license();
}

QString ApplicationService::stripUnreleasedSection(const QString & changeLog)
{
    // A heading is a bare version on its own line with an underline beneath it. Matching on the
    // pair rather than the version alone keeps a version mentioned inside a bullet from cutting the
    // file in the wrong place.
    static const QRegularExpression versionHeading { "^\\d+\\.\\d+\\.\\d+$" };

    const auto lines = changeLog.split(QChar { '\n' });
    for (qsizetype i = 0; i + 1 < lines.size(); i++) {
        if (versionHeading.match(lines.at(i)).hasMatch() && lines.at(i + 1).startsWith("=====")) {
            return lines.mid(i).join(QChar { '\n' });
        }
    }

    return {};
}

QString ApplicationService::changeLog() const
{
    // Read once: the file is bundled, so it cannot change under a running application
    static const QString changeLog = [] {
        QFile file { ":/CHANGELOG" };
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            juzzlin::L(TAG).error() << "Couldn't open the bundled CHANGELOG";
            return QString {};
        }
        return stripUnreleasedSection(QString::fromUtf8(file.readAll()));
    }();

    return changeLog;
}

QString ApplicationService::deviceSettingsExtension() const
{
    return Constants::deviceSettingsExtension();
}

QString ApplicationService::effectSettingsExtension() const
{
    return Constants::effectSettingsExtension();
}

QString ApplicationService::effectRackSettingsExtension() const
{
    return Constants::effectRackSettingsExtension();
}

QString ApplicationService::fileFormatExtension() const
{
    return Constants::fileFormatExtension();
}

QString ApplicationService::midiFileExtension() const
{
    return Constants::midiFileExtension();
}

bool ApplicationService::isMidiFile(const QString & filePath) const
{
    return filePath.endsWith(".mid", Qt::CaseInsensitive) || filePath.endsWith(".midi", Qt::CaseInsensitive);
}

QString ApplicationService::webSiteUrl() const
{
    return Constants::webSiteUrl();
}

QString ApplicationService::samplerDeviceName() const
{
    return Constants::samplerDeviceName();
}

QString ApplicationService::synthDeviceName() const
{
    return Constants::synthDeviceName();
}

QString ApplicationService::wavetableSynthDeviceName() const
{
    return Constants::wavetableSynthDeviceName();
}

QString ApplicationService::fmSynthDeviceName() const
{
    return Constants::fmSynthDeviceName();
}

QString ApplicationService::juSynthDeviceName() const
{
    return Constants::juSynthDeviceName();
}

QString ApplicationService::bassSynthDeviceName() const
{
    return Constants::bassSynthDeviceName();
}

QString ApplicationService::drumSynthDeviceName() const
{
    return Constants::drumSynthDeviceName();
}

QString ApplicationService::drumSynthV2DeviceName() const
{
    return Constants::drumSynthV2DeviceName();
}

QString ApplicationService::fmDrumsDeviceName() const
{
    return Constants::fmDrumsDeviceName();
}

QString ApplicationService::lfoDeviceName() const
{
    return Constants::lfoDeviceName();
}

QString ApplicationService::screenshotDirectory() const
{
    return m_screenshotDirectory;
}

void ApplicationService::setScreenshotDirectory(QString directory)
{
    m_screenshotDirectory = std::move(directory);
}

int ApplicationService::closeAllPopups() const
{
    int closed = 0;
    for (auto && window : QGuiApplication::topLevelWindows()) {
        // By class name rather than by type: QQuickPopup is private API
        for (auto && child : window->findChildren<QObject *>()) {
            if (child->inherits("QQuickPopup") && child->property("visible").toBool()) {
                QMetaObject::invokeMethod(child, "close");
                closed++;
            }
        }
    }
    return closed;
}

bool ApplicationService::saveScreenshot(const QString & name) const
{
    if (m_screenshotDirectory.isEmpty()) {
        return false;
    }
    // The window rather than an item of it: grabbing an item leaves out the popups over it, which
    // is where every dialog lives
    for (auto && window : QGuiApplication::topLevelWindows()) {
        if (const auto quickWindow = qobject_cast<QQuickWindow *>(window); quickWindow && quickWindow->isVisible()) {
            QDir {}.mkpath(m_screenshotDirectory);
            return quickWindow->grabWindow().save(QDir { m_screenshotDirectory }.filePath(name + ".png"));
        }
    }
    return false;
}

QString ApplicationService::pianoSynthDeviceName() const
{
    return Constants::pianoSynthDeviceName();
}

QString ApplicationService::pianoSynthV2DeviceName() const
{
    return Constants::pianoSynthV2DeviceName();
}

QString ApplicationService::pianoSynthV3DeviceName() const
{
    return Constants::pianoSynthV3DeviceName();
}

QString ApplicationService::kick808DeviceName() const
{
    return Constants::kick808DeviceName();
}

QString ApplicationService::stringVoiceDeviceName() const
{
    return Constants::stringVoiceDeviceName();
}

QString ApplicationService::stringVoiceV2DeviceName() const
{
    return Constants::stringVoiceV2DeviceName();
}

QString ApplicationService::stringVoiceV3DeviceName() const
{
    return Constants::stringVoiceV3DeviceName();
}

QString ApplicationService::speechDeviceName() const
{
    return Constants::speechDeviceName();
}

QString ApplicationService::stringEnsembleDeviceName() const
{
    return Constants::stringEnsembleDeviceName();
}

void ApplicationService::acceptUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes accepted";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogAccepted);
}

void ApplicationService::applyAllTrackSettings()
{
    juzzlin::L(TAG).info() << "Apply all track settings";
    emit applyAllTrackSettingsRequested();
}

void ApplicationService::discardUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes discarded";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogDiscarded);
}

void ApplicationService::rejectUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes rejected";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogCanceled);
}

void ApplicationService::requestNewProject()
{
    juzzlin::L(TAG).info() << "'New file' requested";
    m_stateMachine->calculateState(StateMachine::Action::NewProjectRequested);
}

void ApplicationService::requestRecentFilesDialog()
{
    // No need to route through state machine as this takes place only on startup
    juzzlin::L(TAG).info() << "Recent files requested";
    emit recentFilesDialogRequested();
}

void ApplicationService::requestDeviceRackDialog()
{
    juzzlin::L(TAG).info() << "Device Rack requested";
    emit deviceRackDialogRequested();
}

void ApplicationService::requestMasterEffectsDialog()
{
    juzzlin::L(TAG).info() << "Master Effects requested";
    emit masterEffectsDialogRequested();
}

void ApplicationService::requestDrumSynthDialog()
{
    juzzlin::L(TAG).info() << "Drum Synth requested";
    emit drumSynthDialogRequested();
}

void ApplicationService::requestWavetableSynthDialog()
{
    juzzlin::L(TAG).info() << "Wavetable Synth requested";
    emit wavetableSynthDialogRequested();
}

void ApplicationService::requestFmSynthDialog()
{
    juzzlin::L(TAG).info() << "FM Synth requested";
    emit fmSynthDialogRequested();
}

void ApplicationService::requestOpenProject()
{
    juzzlin::L(TAG).info() << "'Open file' requested";
    m_stateMachine->calculateState(StateMachine::Action::OpenProjectRequested);
}

void ApplicationService::requestOpenExample()
{
    juzzlin::L(TAG).info() << "'Open example song' requested";
    m_stateMachine->calculateState(StateMachine::Action::OpenExampleRequested);
}

void ApplicationService::requestQuit()
{
    juzzlin::L(TAG).info() << "Quit requested";
    m_stateMachine->calculateState(StateMachine::Action::QuitSelected);
}

void ApplicationService::requestSaveProject()
{
    juzzlin::L(TAG).info() << "'Save file' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectRequested);
}

void ApplicationService::requestSaveProjectAs()
{
    juzzlin::L(TAG).info() << "'Save file as' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectAsRequested);
}

void ApplicationService::requestSaveProjectAsTemplate()
{
    juzzlin::L(TAG).info() << "'Save file as template' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectAsTemplateRequested);
}

void ApplicationService::requestMidiExportDialog()
{
    // No need to route through state machine as the current data can always be exported
    juzzlin::L(TAG).info() << "MIDI export requested";
    emit midiExportDialogRequested();
}

void ApplicationService::requestAudioRenderDialog()
{
    juzzlin::L(TAG).info() << "Audio render requested";
    emit audioRenderDialogRequested();
}

void ApplicationService::exportMidiFile(QUrl url, quint64 startPosition, quint64 endPosition, bool exportBank, bool exportProgramChange, bool exportMidiCc, bool exportPitchBend, bool forceDrumChannel10, bool autoAssignChannels)
{
    auto fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        fileName = url.toString();
    }
    juzzlin::L(TAG).info() << "MIDI export requested for " << fileName.toStdString() << " from " << startPosition << " to " << endPosition;
    if (!fileName.endsWith(Constants::midiFileExtension())) {
        fileName += Constants::midiFileExtension();
    }
    MidiExportOptions options;
    options.exportBank = exportBank;
    options.exportProgramChange = exportProgramChange;
    options.exportMidiCc = exportMidiCc;
    options.exportPitchBend = exportPitchBend;
    options.forceDrumChannel10 = forceDrumChannel10;
    options.autoAssignChannels = autoAssignChannels;
    emit midiExportRequested(fileName, startPosition, endPosition, options);
}

void ApplicationService::requestMidiImportDialog()
{
    juzzlin::L(TAG).info() << "MIDI import dialog requested";
    emit midiImportDialogRequested();
}

void ApplicationService::importMidiFile(QUrl url, int importMode, int patternLength, bool quantizeNoteOn, bool quantizeNoteOff, bool connectMidiPorts)
{
    auto fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        fileName = url.toString();
    }
    juzzlin::L(TAG).info() << "MIDI import requested for " << fileName.toStdString() << " mode: " << importMode << " length: " << patternLength << " quantizeNoteOn: " << quantizeNoteOn << " quantizeNoteOff: " << quantizeNoteOff << " connectMidiPorts: " << connectMidiPorts;
    emit midiImportRequested(fileName, static_cast<MidiImportMode>(importMode), patternLength, quantizeNoteOn, quantizeNoteOff, connectMidiPorts);
}

void ApplicationService::requestLiveMidiNoteOn(quint8 rawNote, quint8 velocity)
{
    if (!m_editorService) {
        return;
    }
    const auto position = m_editorService->position();
    const auto instrument = m_editorService->instrument(position.track);
    if (!instrument) {
        juzzlin::L(TAG).info() << "No instrument set on track!";
        return;
    }

    if (m_activeLiveNotes.contains(rawNote)) {
        requestLiveMidiNoteOff(rawNote);
    }

    const auto colSettings = m_editorService->columnSettings(position.track, position.column);

    const int instrumentTranspose { instrument->settings().transpose };
    const int columnTranspose { colSettings ? colSettings->transpose : 0 };
    const int totalTranspose { instrumentTranspose + columnTranspose };

    const int transposedRoot { static_cast<int>(rawNote) + totalTranspose };
    if (transposedRoot < 0 || transposedRoot > 127) {
        juzzlin::L(TAG).warning() << "Transposed root note out of MIDI range: " << transposedRoot;
        return;
    }
    const auto rootNote = static_cast<uint8_t>(transposedRoot);

    uint8_t rootVelocity { velocity };
    rootVelocity = Utils::Midi::scaleVelocityByKey(
        rootVelocity,
        rootNote,
        instrument->settings().midiEffects.velocityKeyTrack,
        instrument->settings().midiEffects.velocityKeyTrackOffset);

    if (const int jitter = instrument->settings().midiEffects.velocityJitter; jitter > 0) {
        const int maxDelta { (static_cast<int>(rootVelocity) * jitter) / 100 };
        std::uniform_int_distribution<int> dist { -maxDelta, 0 };
        rootVelocity = static_cast<uint8_t>(std::clamp(static_cast<int>(rootVelocity) + dist(RandomService::generator()), 0, 127));
    }

    ActiveLiveNote activeNote;
    activeNote.instrument = instrument;

    const bool chordEnabled { colSettings && colSettings->chordAutomationSettings.isEnabled() };

    if (chordEnabled && colSettings->chordAutomationSettings.arpeggiator.enabled) {
        std::vector<Arpeggiator::NoteInfo> chordNotes;
        chordNotes.push_back({ rootNote, rootVelocity });

        auto addChordNote = [&](const ColumnSettings::ChordAutomationSettings::ChordNote & chordNote) {
            if (chordNote.offset != 0) {
                const int chordMidi { static_cast<int>(rootNote) + chordNote.offset };
                if (chordMidi >= 0 && chordMidi <= 127) {
                    const int calculatedVel { static_cast<int>(std::round(static_cast<float>(rootVelocity) * (static_cast<float>(chordNote.velocity) / 100.f))) };
                    uint8_t vel { static_cast<uint8_t>(std::clamp(calculatedVel, 1, 127)) };
                    vel = Utils::Midi::scaleVelocityByKey(
                        vel,
                        static_cast<uint8_t>(chordMidi),
                        instrument->settings().midiEffects.velocityKeyTrack,
                        instrument->settings().midiEffects.velocityKeyTrackOffset);
                    if (const int jitter = instrument->settings().midiEffects.velocityJitter; jitter > 0) {
                        const int maxDelta { (static_cast<int>(vel) * jitter) / 100 };
                        std::uniform_int_distribution<int> dist { -maxDelta, 0 };
                        vel = static_cast<uint8_t>(std::clamp(static_cast<int>(vel) + dist(RandomService::generator()), 0, 127));
                    }
                    chordNotes.push_back({ static_cast<uint8_t>(chordMidi), vel });
                }
            }
        };
        addChordNote(colSettings->chordAutomationSettings.note1);
        addChordNote(colSettings->chordAutomationSettings.note2);
        addChordNote(colSettings->chordAutomationSettings.note3);

        const auto arpeggioSequence = Arpeggiator::generate(colSettings->chordAutomationSettings.arpeggiator.pattern, chordNotes);
        if (!arpeggioSequence.empty()) {
            activeNote.arpeggioSequence = arpeggioSequence;
            activeNote.currentArpIndex = 0;

            const auto firstNote = arpeggioSequence[0];
            emit liveNoteOnRequested(instrument, { firstNote.note, firstNote.velocity });
            activeNote.soundingNotes.push_back(firstNote.note);

            const double bpm { m_editorService->song() ? static_cast<double>(m_editorService->song()->beatsPerMinute()) : 120.0 };
            const double msPerBeat { 60000.0 / std::max(1.0, bpm) };
            const auto eventsPerBeat { std::max(uint8_t(1), colSettings->chordAutomationSettings.arpeggiator.eventsPerBeat) };
            const int intervalMs { std::max(10, static_cast<int>(std::round(msPerBeat / static_cast<double>(eventsPerBeat)))) };

            auto * arpTimer = new QTimer { this };
            arpTimer->setInterval(intervalMs);
            connect(arpTimer, &QTimer::timeout, this, [this, rawNote]() {
                if (const auto it = m_activeLiveNotes.find(rawNote); it != m_activeLiveNotes.end() && !it->second.arpeggioSequence.empty()) {
                    auto & noteInfo = it->second;
                    const auto prevNote = noteInfo.arpeggioSequence[noteInfo.currentArpIndex % noteInfo.arpeggioSequence.size()].note;
                    emit liveNoteOffRequested(noteInfo.instrument, { prevNote, 0 });
                    std::erase(noteInfo.soundingNotes, prevNote);

                    noteInfo.currentArpIndex++;
                    const auto nextNote = noteInfo.arpeggioSequence[noteInfo.currentArpIndex % noteInfo.arpeggioSequence.size()];
                    emit liveNoteOnRequested(noteInfo.instrument, { nextNote.note, nextNote.velocity });
                    noteInfo.soundingNotes.push_back(nextNote.note);
                }
            });
            activeNote.arpeggiatorTimer = arpTimer;
            arpTimer->start();
        }
    } else {
        // Play root note
        emit liveNoteOnRequested(instrument, { rootNote, rootVelocity });
        activeNote.soundingNotes.push_back(rootNote);

        if (chordEnabled) {
            auto triggerChordNote = [&](const ColumnSettings::ChordAutomationSettings::ChordNote & chordNote) {
                if (chordNote.offset != 0) {
                    const int chordMidi { static_cast<int>(rootNote) + chordNote.offset };
                    if (chordMidi >= 0 && chordMidi <= 127) {
                        const int calculatedVel { static_cast<int>(std::round(static_cast<float>(rootVelocity) * (static_cast<float>(chordNote.velocity) / 100.f))) };
                        uint8_t vel { static_cast<uint8_t>(std::clamp(calculatedVel, 1, 127)) };
                        vel = Utils::Midi::scaleVelocityByKey(
                            vel,
                            static_cast<uint8_t>(chordMidi),
                            instrument->settings().midiEffects.velocityKeyTrack,
                            instrument->settings().midiEffects.velocityKeyTrackOffset);
                        if (const int jitter = instrument->settings().midiEffects.velocityJitter; jitter > 0) {
                            const int maxDelta { (static_cast<int>(vel) * jitter) / 100 };
                            std::uniform_int_distribution<int> dist { -maxDelta, 0 };
                            vel = static_cast<uint8_t>(std::clamp(static_cast<int>(vel) + dist(RandomService::generator()), 0, 127));
                        }
                        const auto finalNote = static_cast<uint8_t>(chordMidi);
                        if (chordNote.delay <= 0) {
                            emit liveNoteOnRequested(instrument, { finalNote, vel });
                            activeNote.soundingNotes.push_back(finalNote);
                        } else {
                            auto * timer = new QTimer { this };
                            timer->setSingleShot(true);
                            connect(timer, &QTimer::timeout, this, [this, rawNote, instrument, finalNote, vel, timer]() {
                                if (const auto it = m_activeLiveNotes.find(rawNote); it != m_activeLiveNotes.end()) {
                                    emit liveNoteOnRequested(instrument, { finalNote, vel });
                                    it->second.soundingNotes.push_back(finalNote);
                                    std::erase(it->second.delayedTimers, timer);
                                }
                                timer->deleteLater();
                            });
                            activeNote.delayedTimers.push_back(timer);
                            timer->start(chordNote.delay);
                        }
                    }
                }
            };
            triggerChordNote(colSettings->chordAutomationSettings.note1);
            triggerChordNote(colSettings->chordAutomationSettings.note2);
            triggerChordNote(colSettings->chordAutomationSettings.note3);
        }
    }

    m_activeLiveNotes[rawNote] = std::move(activeNote);
}

void ApplicationService::requestLiveMidiNoteOff(quint8 rawNote)
{
    if (const auto it = m_activeLiveNotes.find(rawNote); it != m_activeLiveNotes.end()) {
        auto & noteInfo = it->second;

        for (auto * timer : noteInfo.delayedTimers) {
            timer->stop();
            timer->deleteLater();
        }
        noteInfo.delayedTimers.clear();

        if (noteInfo.arpeggiatorTimer) {
            noteInfo.arpeggiatorTimer->stop();
            noteInfo.arpeggiatorTimer->deleteLater();
            noteInfo.arpeggiatorTimer = nullptr;
        }

        for (const auto note : noteInfo.soundingNotes) {
            emit liveNoteOffRequested(noteInfo.instrument, { note, 0 });
        }
        noteInfo.soundingNotes.clear();

        m_activeLiveNotes.erase(it);
    } else {
        if (!m_editorService) {
            return;
        }
        if (const auto instrument = m_editorService->instrument(m_editorService->position().track); instrument) {
            const auto colSettings = m_editorService->columnSettings(m_editorService->position().track, m_editorService->position().column);
            const int totalTranspose { instrument->settings().transpose + (colSettings ? colSettings->transpose : 0) };
            const int transposed { static_cast<int>(rawNote) + totalTranspose };
            if (transposed >= 0 && transposed <= 127) {
                emit liveNoteOffRequested(instrument, { static_cast<uint8_t>(transposed), 0 });
            }
        }
    }
}

void ApplicationService::requestLiveNoteOn(quint8 key, quint8 octave, quint8 velocity)
{
    if (key < 1 || key > 12) {
        return;
    }
    const auto rawNote = static_cast<uint8_t>(octave * 12 + (key - 1));
    requestLiveMidiNoteOn(rawNote, velocity);
}

void ApplicationService::requestLiveNoteOnAtCurrentPosition()
{
    if (!m_editorService) {
        return;
    }
    const auto instrument = m_editorService->instrument(m_editorService->position().track);
    if (!instrument) {
        juzzlin::L(TAG).info() << "No instrument set on track!";
        return;
    }
    juzzlin::L(TAG).debug() << "Live note ON requested on instrument " << instrument->toString().toStdString();
    emit liveNoteOnAtCurrentPositionRequested(instrument);
    if (m_editorService->song()) {
        if (const auto noteData = m_editorService->song()->noteDataAtPosition(m_editorService->position()); noteData && noteData->note().has_value()) {
            requestLiveMidiNoteOn(*noteData->note(), noteData->velocity());
        }
    }
}

void ApplicationService::requestInstrumentReset()
{
    juzzlin::L(TAG).info() << "Instrument reset requested";
    emit applyAllTrackSettingsRequested();
}

void ApplicationService::stopAllLiveNotes()
{
    for (auto & [rawNote, noteInfo] : m_activeLiveNotes) {
        for (auto * timer : noteInfo.delayedTimers) {
            timer->stop();
            timer->deleteLater();
        }
        noteInfo.delayedTimers.clear();

        if (noteInfo.arpeggiatorTimer) {
            noteInfo.arpeggiatorTimer->stop();
            noteInfo.arpeggiatorTimer->deleteLater();
            noteInfo.arpeggiatorTimer = nullptr;
        }

        for (const auto note : noteInfo.soundingNotes) {
            emit liveNoteOffRequested(noteInfo.instrument, { note, 0 });
        }
        noteInfo.soundingNotes.clear();
    }
    m_activeLiveNotes.clear();
}

void ApplicationService::requestAllNotesOff()
{
    juzzlin::L(TAG).info() << "All notes off requested";
    stopAllLiveNotes();
    emit allNotesOffRequested();
}

void ApplicationService::requestLiveNoteOff(quint8 key, quint8 octave)
{
    if (key < 1 || key > 12) {
        return;
    }
    const auto rawNote = static_cast<uint8_t>(octave * 12 + (key - 1));
    requestLiveMidiNoteOff(rawNote);
}

void ApplicationService::requestLiveNoteOffAtCurrentPosition()
{
    if (!m_editorService) {
        return;
    }
    const auto instrument = m_editorService->instrument(m_editorService->position().track);
    if (!instrument) {
        juzzlin::L(TAG).info() << "No instrument set on track!";
        return;
    }
    juzzlin::L(TAG).debug() << "Live note OFF at current position requested on instrument " << instrument->toString().toStdString();
    emit liveNoteOffAtCurrentPositionRequested(instrument);
    if (m_editorService->song()) {
        if (const auto noteData = m_editorService->song()->noteDataAtPosition(m_editorService->position()); noteData && noteData->note().has_value()) {
            requestLiveMidiNoteOff(*noteData->note());
        }
    }
}

void ApplicationService::cancelOpenProject()
{
    m_stateMachine->calculateState(StateMachine::Action::OpeningProjectCanceled);
}

void ApplicationService::openProject(QUrl url)
{
    try {
        m_editorService->load(url.toLocalFile());
        m_recentFilesManager->addRecentFile(url.toLocalFile());
        m_stateMachine->calculateState(StateMachine::Action::ProjectOpened);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to load project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::openRecentProject(QString filePath)
{
    m_recentFilesManager->setSelectedFile(filePath);

    m_stateMachine->calculateState(StateMachine::Action::RecentFileSelected);
}

void ApplicationService::cancelRecentFileDialog()
{
    m_stateMachine->calculateState(StateMachine::Action::RecentFileDialogCanceled);
}

void ApplicationService::cancelSaveProjectAs()
{
    m_stateMachine->calculateState(StateMachine::Action::SavingProjectAsCanceled);
}

void ApplicationService::saveProjectAs(QUrl url)
{
    try {
        auto fileName = url.toLocalFile();
        if (!fileName.endsWith(Constants::fileFormatExtension())) {
            fileName += Constants::fileFormatExtension();
        }
        m_editorService->saveAs(fileName);
        m_recentFilesManager->addRecentFile(fileName);
        m_stateMachine->calculateState(StateMachine::Action::ProjectSavedAs);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to save project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::saveProjectAsTemplate(QUrl url)
{
    try {
        auto fileName = url.toLocalFile();
        if (!fileName.endsWith(Constants::fileFormatExtension())) {
            fileName += Constants::fileFormatExtension();
        }
        m_editorService->saveAsTemplate(fileName);
        m_recentFilesManager->addRecentFile(fileName);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to save project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

QStringList ApplicationService::recentFiles() const
{
    return m_recentFilesManager->recentFiles();
}

void ApplicationService::addRecentFile(QString filePath)
{
    m_recentFilesManager->addRecentFile(filePath);
}

QString ApplicationService::selectedFile() const
{
    return m_recentFilesManager->selectedFile();
}

QString ApplicationService::lastImportDirectory() const
{
    return m_recentFilesManager->lastImportDirectory();
}

void ApplicationService::setLastImportDirectory(QString directory)
{
    m_recentFilesManager->setLastImportDirectory(directory);
}

QString ApplicationService::lastEffectImportDirectory() const
{
    return m_recentFilesManager->lastEffectImportDirectory();
}

void ApplicationService::setLastEffectImportDirectory(QUrl url)
{
    const auto fileInfo = QFileInfo { url.toLocalFile() };
    m_recentFilesManager->setLastEffectImportDirectory(fileInfo.absolutePath());
}

QString ApplicationService::lastEffectExportDirectory() const
{
    return m_recentFilesManager->lastEffectExportDirectory();
}

void ApplicationService::setLastEffectExportDirectory(QUrl url)
{
    const auto fileInfo = QFileInfo { url.toLocalFile() };
    m_recentFilesManager->setLastEffectExportDirectory(fileInfo.absolutePath());
}

QString ApplicationService::defaultDeviceFileName(const QString & deviceName) const
{
    const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString { "%1-%2" }.arg(deviceName, date);
}

QString ApplicationService::defaultEffectFileName(const QString & effectType) const
{
    const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString { "%1-%2" }.arg(effectType, date);
}

QString ApplicationService::defaultEffectRackFileName(const QString & rackName) const
{
    const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString { "%1-%2" }.arg(rackName, date);
}

QUrl ApplicationService::fromLocalFile(const QString & localPath) const
{
    return QUrl::fromLocalFile(localPath);
}

void ApplicationService::requestUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Not saved dialog requested";
    emit unsavedChangesDialogRequested();
}

void ApplicationService::requestOpenDialog()
{
    juzzlin::L(TAG).info() << "Open dialog requested";
    emit openDialogRequested();
}

void ApplicationService::requestSaveAsDialog()
{
    juzzlin::L(TAG).info() << "Save as dialog requested";
    emit saveAsDialogRequested();
}

void ApplicationService::requestSaveAsTemplateDialog()
{
    juzzlin::L(TAG).info() << "Save as template dialog requested";
    emit saveAsTemplateDialogRequested();
}

void ApplicationService::requestAlertDialog(QString text)
{
    emit alertDialogRequested(text);
}

void ApplicationService::requestLoudnessReportDialog(QString text)
{
    emit loudnessReportDialogRequested(text);
}

void ApplicationService::requestStatusText(QString text)
{
    emit statusTextRequested(text);
}

void ApplicationService::setRecentFilesManager(RecentFilesManagerS recentFilesManager)
{
    m_recentFilesManager = recentFilesManager;
}

void ApplicationService::setStateMachine(StateMachineS stateMachine)
{
    m_stateMachine = stateMachine;
}

void ApplicationService::setEditorService(EditorServiceS editorService)
{
    m_editorService = editorService;
}

void ApplicationService::setPlayerService(PlayerServiceS playerService)
{
    m_playerService = playerService;
}

bool ApplicationService::editMode() const
{
    return m_editMode;
}

void ApplicationService::setEditMode(bool editMode)
{
    if (m_editMode != editMode) {
        m_editMode = editMode;
        emit editModeChanged(m_editMode);
    }
}

void ApplicationService::toggleEditMode()
{
    setEditMode(!m_editMode);
}

QString ApplicationService::initialFilePath() const
{
    return m_initialFilePath;
}

void ApplicationService::setInitialFilePath(QString initialFilePath)
{
    m_initialFilePath = initialFilePath;
}

ApplicationService::~ApplicationService()
{
    stopAllLiveNotes();
}

} // namespace noteahead
