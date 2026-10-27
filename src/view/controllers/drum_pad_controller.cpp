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

#include "drum_pad_controller.hpp"

#include <QVariant>

#include "../../application/service/device_service.hpp"
#include "../../application/service/drum_voice_preview.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/drum_pad_device.hpp"
#include "../../domain/devices/drum_synth_v2_constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

DrumPadController::DrumPadController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DeviceController { parent }
  , m_deviceService { std::move(deviceService) }
{
    m_waveformTimer.setSingleShot(true);
    m_waveformTimer.setInterval(WaveformDebounceMs);
    connect(&m_waveformTimer, &QTimer::timeout, this, &DrumPadController::renderWaveform);
}

DeviceController::DeviceS DrumPadController::device() const
{
    return m_device;
}

bool DrumPadController::setDevice(DeviceS device)
{
    if (const auto drumSynth = std::dynamic_pointer_cast<DrumPadDevice>(device); drumSynth && acceptsDevice(device)) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = drumSynth;
        connectDeviceSignals();
        requestSettings();
        return true;
    }
    return false;
}

void DrumPadController::setDevice(const QString & deviceName)
{
    setDevice(m_deviceService->device(deviceName.toStdString()));
}

int DrumPadController::lpfSlope() const
{
    return m_device ? m_device->lpfSlope() : 0;
}

void DrumPadController::setLpfSlope(int value)
{
    if (m_device) {
        m_device->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int DrumPadController::hpfSlope() const
{
    return m_device ? m_device->hpfSlope() : 0;
}

void DrumPadController::setHpfSlope(int value)
{
    if (m_device) {
        m_device->setHpfSlope(value);
        emit hpfSlopeChanged();
    }
}

int DrumPadController::selectedVoice() const
{
    return m_selectedVoice;
}

void DrumPadController::setSelectedVoice(int index)
{
    if (m_selectedVoice != index) {
        m_selectedVoice = index;
        emit selectedVoiceChanged();
        requestSettings();
        // Without waiting, but not here either. Rendering a voice takes a third of a second for a
        // crash, and the pad that changed this also plays the note: doing it inside this call
        // leaves the drum silent until the picture is finished. Zero puts it on the next turn of
        // the event loop, by which time the click has been served and the note has sounded.
        scheduleWaveform(0);
    }
}

double DrumPadController::voiceAmpAttackSeconds() const
{
    return ParameterMapper::mapCubic(static_cast<double>(voiceAmpAttack()) / Constants::uiInternalScaling(), 0.0, DrumSynthV2::AmpEnvelopeMaxAttackSeconds);
}

double DrumPadController::voiceAmpHoldSeconds() const
{
    return ParameterMapper::mapCubic(static_cast<double>(voiceAmpHold()) / Constants::uiInternalScaling(), 0.0, DrumSynthV2::AmpEnvelopeMaxHoldSeconds);
}

double DrumPadController::voiceAmpDecaySeconds() const
{
    return ParameterMapper::mapExponential(static_cast<double>(voiceAmpDecay()) / Constants::uiInternalScaling(), DrumSynthV2::AmpEnvelopeMinDecaySeconds, DrumSynthV2::AmpEnvelopeMaxDecaySeconds);
}

double DrumPadController::voiceAmpReleaseSeconds() const
{
    return ParameterMapper::mapExponential(static_cast<double>(voiceAmpRelease()) / Constants::uiInternalScaling(), DrumSynthV2::AmpEnvelopeMinDecaySeconds, DrumSynthV2::AmpEnvelopeMaxDecaySeconds);
}

double DrumPadController::playbackPosition() const
{
    if (!m_device || m_waveformDuration <= 0.0) {
        return 0.0;
    }
    const auto elapsed = m_device->voiceElapsedSeconds(m_selectedVoice);
    if (!elapsed) {
        return 0.0;
    }
    // Against the picture rather than against the sound: a voice whose envelope has been pulled in
    // stops before the end of the material, and the playhead has to stop where it is heard to.
    return std::clamp(*elapsed / m_waveformDuration, 0.0, 1.0);
}

bool DrumPadController::voiceSounding() const
{
    return m_device && m_device->voiceElapsedSeconds(m_selectedVoice).has_value();
}

void DrumPadController::updatePlaybackStatus()
{
    emit playbackStatusChanged();
}

QVariantList DrumPadController::waveformData() const
{
    return m_waveformData;
}

double DrumPadController::waveformDuration() const
{
    return m_waveformDuration;
}

double DrumPadController::audibleLength() const
{
    return m_audibleLength;
}

void DrumPadController::setWaveformRequest(int peakCount, bool visible)
{
    // A resize changes the picture as surely as a parameter does, so it goes through the same wait:
    // dragging a dialog edge would otherwise render on every frame of the drag.
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void DrumPadController::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    // Restarted rather than left running, which is what collapses a drag into one render.
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void DrumPadController::renderWaveform()
{
    m_waveformTimer.stop();
    if (!m_device || !m_waveformVisible || m_waveformPeakCount <= 0) {
        return;
    }

    const auto preview = DrumVoicePreview::render(*m_device, m_selectedVoice, m_waveformPeakCount, Constants::defaultSampleRate());
    m_waveformData = preview.peaks;
    m_waveformDuration = preview.durationSeconds;
    m_audibleLength = preview.audibleSeconds;
    m_waveformEnvelope = {
        .attack = voiceAmpAttackSeconds(),
        .hold = voiceAmpHoldSeconds(),
        .decay = voiceAmpDecaySeconds(),
        .sustain = static_cast<double>(voiceAmpSustain()) / Constants::uiInternalScaling(),
        .release = voiceAmpReleaseSeconds(),
        .curve = static_cast<double>(voiceAmpCurve()) / Constants::uiInternalScaling()
    };
    emit waveformChanged();
}

double DrumPadController::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double DrumPadController::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double DrumPadController::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double DrumPadController::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double DrumPadController::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double DrumPadController::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

int DrumPadController::voiceLevel() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyFader().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceLevel(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyFader().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voicePan() const
{
    if (!m_device) {
        return 500;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyPan().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 500;
}

void DrumPadController::setVoicePan(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyPan().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceLpfCutoff() const
{
    if (!m_device) {
        return 1000;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 1000;
}

void DrumPadController::setVoiceLpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceHpfCutoff() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyHpfCutoff().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceHpfCutoff(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpAttack() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpAttack().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpAttack(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpAttack().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpHold() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpHold().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpHold(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpHold().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpSustain() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpSustain().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpSustain(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpRelease() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpRelease().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpRelease(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpRelease().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpDecay() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpDecay().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpDecay(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int DrumPadController::voiceAmpCurve() const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + Constants::NahdXml::xmlKeyAmpCurve().toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumPadController::setVoiceAmpCurve(int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyAmpCurve().toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

QVariantList DrumPadController::activeNotes() const
{
    QVariantList list;
    if (m_device) {
        for (int i = 0; i < DrumSynthV2::NumVoices; i++) {
            if (const uint8_t note = m_device->voiceNote(i); note > 0) {
                list.append(note);
            }
        }
        // Pedal HiHat is not a separate voice but it is an active note
        list.append(static_cast<uint8_t>(DrumSynthV2::MidiNote::PedalHiHat));
    }
    return list;
}

void DrumPadController::playVoice(int index)
{
    if (m_device) {
        if (const uint8_t note = m_device->voiceNote(index); note > 0) {
            playNote(note, 1.0);
        }
    }
}

int DrumPadController::voiceCount() const
{
    return DrumSynthV2::NumVoices;
}

QString DrumPadController::voiceName(int index) const
{
    return DrumSynthV2::voiceName(index);
}

std::string DrumPadController::currentVoicePrefix() const
{
    return DrumSynthV2::voiceId(m_selectedVoice) + "_";
}

void DrumPadController::requestSettings()
{
    // Everything that changes a voice arrives here: the dialog's own knobs through the device's
    // dataChanged, and so do presets, automation and incoming MIDI CC. Hooking the redraw on this
    // one signal is what keeps the picture honest without a call in every setter.
    scheduleWaveform();

    emit selectedVoiceChanged();
    emit voiceLevelChanged();
    emit voicePanChanged();
    emit voiceLpfCutoffChanged();
    emit voiceHpfCutoffChanged();
    emit lpfSlopeChanged();
    emit hpfSlopeChanged();
    emit voiceAmpAttackChanged();
    emit voiceAmpHoldChanged();
    emit voiceAmpDecayChanged();
    emit voiceAmpSustainChanged();
    emit voiceAmpReleaseChanged();
    emit voiceAmpCurveChanged();
    emit activeNotesChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
