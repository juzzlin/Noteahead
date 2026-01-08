// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "track_settings_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"

#include <iomanip>

namespace noteahead {

static const auto TAG = "TrackSettingsModel";

TrackSettingsModel::TrackSettingsModel(QObject * parent)
  : QObject { parent }
  , m_midiCcModel { new MidiCcSelectionModel(this) }
{
}

MidiCcSelectionModel* TrackSettingsModel::midiCcModel() const
{
    return m_midiCcModel;
}

void TrackSettingsModel::applyAll()
{
    if (!m_applyDisabled && !m_instrumentSettings.portName.isEmpty()) {
        emit applyAllRequested();
    }
}

void TrackSettingsModel::applyMidiCc()
{
    if (!m_applyDisabled && !m_instrumentSettings.portName.isEmpty()) {
        emit applyMidiCcRequested();
    }
}

void TrackSettingsModel::requestInstrumentData()
{
    emit instrumentDataRequested();
}

void TrackSettingsModel::requestNoteOff(quint8 note)
{
    emit noteOffRequested(note);
}

void TrackSettingsModel::requestNoteOn(quint8 note, quint8 velocity)
{
    emit noteOnRequested(note, velocity);
}

void TrackSettingsModel::requestTestSound(quint8 velocity)
{
    emit testSoundRequested(velocity);
}

void TrackSettingsModel::save()
{
    emit saveRequested();
}

QStringList TrackSettingsModel::availableMidiPorts() const
{
    return m_availableMidiPorts;
}

void TrackSettingsModel::setAvailableMidiPorts(QStringList portNames)
{
    pushApplyDisabled();

    const auto oldMidiPorts = m_availableMidiPorts;
    m_availableMidiPorts = portNames;
    if (!m_instrumentPortName.isEmpty() && !m_availableMidiPorts.contains(m_instrumentPortName)) {
        m_availableMidiPorts.append(m_instrumentPortName);
    }

    if (m_availableMidiPorts != oldMidiPorts) {
        juzzlin::L(TAG).info() << "Setting available MIDI ports to '" << m_availableMidiPorts.join(", ").toStdString() << "'";
        emit availableMidiPortsChanged();
    }

    popApplyDisabled();
}

quint64 TrackSettingsModel::trackIndex() const
{
    return m_trackIndex;
}

quint8 TrackSettingsModel::channel() const
{
    return m_instrumentSettings.channel;
}

void TrackSettingsModel::setChannel(quint8 channel)
{
    juzzlin::L(TAG).debug() << "Setting channel to " << static_cast<int>(channel);

    if (m_instrumentSettings.channel != channel) {
        m_instrumentSettings.channel = channel;
        emit channelChanged();
        applyAll();
    }
}

bool TrackSettingsModel::bankEnabled() const
{
    return m_instrumentSettings.bankEnabled;
}

void TrackSettingsModel::setBankEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling bank: " << static_cast<int>(enabled);

    if (m_instrumentSettings.bankEnabled != enabled) {
        m_instrumentSettings.bankEnabled = enabled;
        emit bankEnabledChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::bankLsb() const
{
    return m_instrumentSettings.bankLsb;
}

void TrackSettingsModel::setBankLsb(quint8 lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_instrumentSettings.bankLsb != lsb) {
        m_instrumentSettings.bankLsb = lsb;
        emit bankLsbChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::bankMsb() const
{
    return m_instrumentSettings.bankMsb;
}

void TrackSettingsModel::setBankMsb(quint8 msb)
{
    juzzlin::L(TAG).debug() << "Setting bank MSB to " << static_cast<int>(msb);

    if (m_instrumentSettings.bankMsb != msb) {
        m_instrumentSettings.bankMsb = msb;
        emit bankMsbChanged();
        applyAll();
    }
}

bool TrackSettingsModel::bankByteOrderSwapped() const
{
    return m_instrumentSettings.bankByteOrderSwapped;
}

void TrackSettingsModel::setBankByteOrderSwapped(bool swapped)
{
    juzzlin::L(TAG).debug() << "Enabling swapped bank byte order: " << static_cast<int>(swapped);

    if (m_instrumentSettings.bankByteOrderSwapped != swapped) {
        m_instrumentSettings.bankByteOrderSwapped = swapped;
        emit bankByteOrderSwappedChanged();
        applyAll();
    }
}

void TrackSettingsModel::setTrackIndex(quint64 trackIndex)
{
    juzzlin::L(TAG).info() << "Setting track index to " << trackIndex;

    if (m_trackIndex != trackIndex) {
        m_trackIndex = trackIndex;
        emit trackIndexChanged();
    }
}

void TrackSettingsModel::setInstrumentData(const Instrument & instrument)
{
    juzzlin::L(TAG).info() << "Setting instrument data: " << instrument.toString().toStdString();

    pushApplyDisabled();

    m_instrumentPortName = instrument.midiAddress().portName();
    setAvailableMidiPorts(m_availableMidiPorts);

    setPortName(instrument.midiAddress().portName());
    setChannel(instrument.midiAddress().channel());
    setPatchEnabled(instrument.settings().patch.has_value());
    if (patchEnabled()) {
        setPatch(*instrument.settings().patch);
    }
    setBankEnabled(instrument.settings().bank.has_value());
    if (bankEnabled()) {
        setBankLsb(instrument.settings().bank->lsb);
        setBankMsb(instrument.settings().bank->msb);
        setBankByteOrderSwapped(instrument.settings().bank->byteOrderSwapped);
    }
    setTranspose(instrument.settings().transpose);

    setSendMidiClock(instrument.settings().timing.sendMidiClock.has_value() && *instrument.settings().timing.sendMidiClock);
    setSendTransport(instrument.settings().timing.sendTransport.has_value() && *instrument.settings().timing.sendTransport);
    setAutoNoteOffOffsetEnabled(instrument.settings().timing.autoNoteOffOffset.has_value());
    if (autoNoteOffOffsetEnabled()) {
        setAutoNoteOffOffset(static_cast<int>(instrument.settings().timing.autoNoteOffOffset.value().count()));
    }
    setDelay(static_cast<int>(instrument.settings().timing.delay.count()));

    setVelocityJitter(instrument.settings().midiEffects.velocityJitter);
    setVelocityKeyTrack(instrument.settings().midiEffects.velocityKeyTrack);

    m_midiCcModel->setMidiCcSettings(instrument.settings().midiCcSettings);

    emit instrumentDataReceived();

    popApplyDisabled();
}

void TrackSettingsModel::reset()
{
    juzzlin::L(TAG).info() << "Reset";

    pushApplyDisabled();

    m_instrumentPortName = {};
    setAvailableMidiPorts(m_availableMidiPorts); 

    m_instrumentSettings = {};

    m_timingSettings = {};

    m_midiEffectSettings = {};

    m_midiCcModel->setMidiCcSettings({});

    emit instrumentDataReceived();

    popApplyDisabled();
}

TrackSettingsModel::InstrumentU TrackSettingsModel::toInstrument() const
{
    auto instrument = std::make_unique<Instrument>(m_instrumentSettings.portName);

    auto address = instrument->midiAddress();
    address.setChannel(m_instrumentSettings.channel);
    instrument->setMidiAddress(address);

    auto settings = instrument->settings();
    if (m_instrumentSettings.patchEnabled) {
        settings.patch = m_instrumentSettings.patch;
    }
    if (m_instrumentSettings.bankEnabled) {
        settings.bank = {
            m_instrumentSettings.bankLsb,
            m_instrumentSettings.bankMsb,
            m_instrumentSettings.bankByteOrderSwapped
        };
    }

    settings.transpose = m_instrumentSettings.transpose;

    settings.timing.sendMidiClock = m_timingSettings.sendMidiClock;
    settings.timing.sendTransport = m_timingSettings.sendTransport;
    settings.timing.delay = std::chrono::milliseconds { m_timingSettings.delay };

    if (m_timingSettings.autoNoteOffOffsetEnabled) {
        settings.timing.autoNoteOffOffset = std::chrono::milliseconds { m_timingSettings.autoNoteOffOffset };
    }

    settings.midiEffects.velocityJitter = m_midiEffectSettings.velocityJitter;
    settings.midiEffects.velocityKeyTrack = m_midiEffectSettings.velocityKeyTrack;

    settings.midiCcSettings = m_midiCcModel->midiCcSettings();
    instrument->setSettings(settings);

    return instrument;
}

QString TrackSettingsModel::portName() const
{
    return m_instrumentSettings.portName;
}

void TrackSettingsModel::setPortName(const QString & name)
{
    juzzlin::L(TAG).debug() << "Setting port name to " << std::quoted(name.toStdString());

    if (m_instrumentSettings.portName != name) {
        m_instrumentSettings.portName = name;
        emit portNameChanged();
        applyAll();
    }
}

bool TrackSettingsModel::patchEnabled() const
{
    return m_instrumentSettings.patchEnabled;
}

void TrackSettingsModel::setPatchEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling patch: " << static_cast<int>(enabled);

    if (m_instrumentSettings.patchEnabled != enabled) {
        m_instrumentSettings.patchEnabled = enabled;
        emit patchEnabledChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::patch() const
{
    return m_instrumentSettings.patch;
}

void TrackSettingsModel::setPatch(quint8 patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_instrumentSettings.patch != patch) {
        m_instrumentSettings.patch = patch;
        emit patchChanged();
        applyAll();
    }
}

bool TrackSettingsModel::sendMidiClock() const
{
    return m_timingSettings.sendMidiClock;
}

void TrackSettingsModel::setSendMidiClock(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling MIDI clock: " << static_cast<int>(enabled);

    if (m_timingSettings.sendMidiClock != enabled) {
        m_timingSettings.sendMidiClock = enabled;
        emit sendMidiClockChanged();
        applyAll();
    }
}

bool TrackSettingsModel::sendTransport() const
{
    return m_timingSettings.sendTransport;
}

void TrackSettingsModel::setSendTransport(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling transport: " << static_cast<int>(enabled);

    if (m_timingSettings.sendTransport != enabled) {
        m_timingSettings.sendTransport = enabled;
        emit sendTransportChanged();
        applyAll();
    }
}

int TrackSettingsModel::delay() const
{
    return m_timingSettings.delay;
}

void TrackSettingsModel::setDelay(int delay)
{
    juzzlin::L(TAG).debug() << "Setting delay to " << delay;

    if (m_timingSettings.delay != delay) {
        m_timingSettings.delay = delay;
        emit delayChanged();
    }
}

int TrackSettingsModel::transpose() const
{
    return m_instrumentSettings.transpose;
}

void TrackSettingsModel::setTranspose(int transpose)
{
    juzzlin::L(TAG).debug() << "Setting transposition to " << transpose;

    if (m_instrumentSettings.transpose != transpose) {
        m_instrumentSettings.transpose = transpose;
        emit transposeChanged();
    }
}

int TrackSettingsModel::velocityJitter() const
{
    return m_midiEffectSettings.velocityJitter;
}

void TrackSettingsModel::setVelocityJitter(int velocityJitter)
{
    juzzlin::L(TAG).debug() << "Setting velocty jitter to " << velocityJitter;

    if (m_midiEffectSettings.velocityJitter != velocityJitter) {
        m_midiEffectSettings.velocityJitter = velocityJitter;
        emit velocityJitterChanged();
    }
}

int TrackSettingsModel::velocityKeyTrack() const
{
    return m_midiEffectSettings.velocityKeyTrack;
}

void TrackSettingsModel::setVelocityKeyTrack(int velocityKeyTrack)
{
    juzzlin::L(TAG).debug() << "Setting velocty key track to " << velocityKeyTrack;

    if (m_midiEffectSettings.velocityKeyTrack != velocityKeyTrack) {
        m_midiEffectSettings.velocityKeyTrack = velocityKeyTrack;
        emit velocityKeyTrackChanged();
    }
}

int TrackSettingsModel::autoNoteOffOffset() const
{
    return m_timingSettings.autoNoteOffOffset;
}

void TrackSettingsModel::setAutoNoteOffOffset(int autoNoteOffOffset)
{
    juzzlin::L(TAG).debug() << "Setting auto note-off offset to " << autoNoteOffOffset;

    if (m_timingSettings.autoNoteOffOffset != autoNoteOffOffset) {
        m_timingSettings.autoNoteOffOffset = autoNoteOffOffset;
        emit autoNoteOffOffsetChanged();
    }
}

bool TrackSettingsModel::autoNoteOffOffsetEnabled() const
{
    return m_timingSettings.autoNoteOffOffsetEnabled;
}

void TrackSettingsModel::setAutoNoteOffOffsetEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling auto note-off offset: " << static_cast<int>(enabled);

    if (m_timingSettings.autoNoteOffOffsetEnabled != enabled) {
        m_timingSettings.autoNoteOffOffsetEnabled = enabled;
        emit autoNoteOffOffsetEnabledChanged();
        applyAll();
    }
}


void TrackSettingsModel::pushApplyDisabled()
{
    m_applyDisabledStack.push_back(m_applyDisabled);
    m_applyDisabled = true;
}

void TrackSettingsModel::popApplyDisabled()
{
    if (!m_applyDisabledStack.empty()) {
        m_applyDisabled = m_applyDisabledStack.back();
        m_applyDisabledStack.pop_back();
    }
}

TrackSettingsModel::~TrackSettingsModel() = default;

} // namespace noteahead