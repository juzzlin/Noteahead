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
  : MidiCcSelectionModel { parent }
{
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

quint8 TrackSettingsModel::cutoff() const
{
    return m_cutoff;
}

void TrackSettingsModel::setCutoff(quint8 cutoff)
{
    juzzlin::L(TAG).debug() << "Setting cutoff to " << static_cast<int>(cutoff);

    if (m_cutoff != cutoff) {
        m_cutoff = cutoff;
        emit cutoffChanged();
        applyAll();
    }
}

bool TrackSettingsModel::cutoffEnabled() const
{
    return m_cutoffEnabled;
}

void TrackSettingsModel::setCutoffEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling cutoff: " << static_cast<int>(enabled);

    if (m_cutoffEnabled != enabled) {
        m_cutoffEnabled = enabled;
        emit cutoffEnabledChanged();
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

    // Store the original instrument's port name as it might not be in the
    // list of currently available port names
    m_instrumentPortName = instrument.midiAddress().portName();
    setAvailableMidiPorts(m_availableMidiPorts); // Update the list with instrument port name

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

    setCutoffEnabled(instrument.settings().standardMidiCcSettings.cutoff.has_value());
    if (cutoffEnabled()) {
        setCutoff(*instrument.settings().standardMidiCcSettings.cutoff);
    }
    setPanEnabled(instrument.settings().standardMidiCcSettings.pan.has_value());
    if (panEnabled()) {
        setPan(*instrument.settings().standardMidiCcSettings.pan);
    }
    setVolumeEnabled(instrument.settings().standardMidiCcSettings.volume.has_value());
    if (volumeEnabled()) {
        setVolume(*instrument.settings().standardMidiCcSettings.volume);
    }

    setSendMidiClock(instrument.settings().timing.sendMidiClock.has_value() && *instrument.settings().timing.sendMidiClock);
    setSendTransport(instrument.settings().timing.sendTransport.has_value() && *instrument.settings().timing.sendTransport);
    setAutoNoteOffOffsetEnabled(instrument.settings().timing.autoNoteOffOffset.has_value());
    if (autoNoteOffOffsetEnabled()) {
        setAutoNoteOffOffset(static_cast<int>(instrument.settings().timing.autoNoteOffOffset.value().count()));
    }
    setDelay(static_cast<int>(instrument.settings().timing.delay.count()));

    setVelocityJitter(instrument.settings().midiEffects.velocityJitter);

    setMidiCcSettings(instrument.settings().midiCcSettings);

    emit instrumentDataReceived();

    popApplyDisabled();
}

void TrackSettingsModel::reset()
{
    juzzlin::L(TAG).info() << "Reset";

    pushApplyDisabled();

    m_instrumentPortName = {};
    setAvailableMidiPorts(m_availableMidiPorts); // Update the list with instrument port name

    m_instrumentSettings = {};

    m_autoNoteOffOffset = {};
    m_autoNoteOffOffsetEnabled = false;
    m_cutoff = m_defaultCutoff;
    m_cutoffEnabled = false;
    m_delay = 0;
    m_pan = m_defaultPan;
    m_panEnabled = false;
    m_sendMidiClock = false;
    m_sendTransport = false;
    m_velocityJitter = 0;
    m_volume = m_defaultVolume;
    m_volumeEnabled = false;

    setMidiCcSettings({});

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

    if (m_cutoffEnabled) {
        settings.standardMidiCcSettings.cutoff = m_cutoff;
    }
    if (m_panEnabled) {
        settings.standardMidiCcSettings.pan = m_pan;
    }
    if (m_volumeEnabled) {
        settings.standardMidiCcSettings.volume = m_volume;
    }

    settings.timing.sendMidiClock = m_sendMidiClock;
    settings.timing.sendTransport = m_sendTransport;
    settings.timing.delay = std::chrono::milliseconds { m_delay };

    if (m_autoNoteOffOffsetEnabled) {
        settings.timing.autoNoteOffOffset = std::chrono::milliseconds { m_autoNoteOffOffset };
    }

    settings.midiEffects.velocityJitter = m_velocityJitter;

    settings.midiCcSettings = midiCcSettings();
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

quint8 TrackSettingsModel::pan() const
{
    return m_pan;
}

void TrackSettingsModel::setPan(quint8 pan)
{
    juzzlin::L(TAG).debug() << "Setting pan to " << static_cast<int>(pan);

    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged();
        applyAll();
    }
}

bool TrackSettingsModel::panEnabled() const
{
    return m_panEnabled;
}

void TrackSettingsModel::setPanEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling pan: " << static_cast<int>(enabled);

    if (m_panEnabled != enabled) {
        m_panEnabled = enabled;
        emit panEnabledChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::volume() const
{
    return m_volume;
}

void TrackSettingsModel::setVolume(quint8 volume)
{
    juzzlin::L(TAG).debug() << "Setting volume to " << static_cast<int>(volume);

    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged();
        applyAll();
    }
}

bool TrackSettingsModel::volumeEnabled() const
{
    return m_volumeEnabled;
}

void TrackSettingsModel::setVolumeEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling volume: " << static_cast<int>(enabled);

    if (m_volumeEnabled != enabled) {
        m_volumeEnabled = enabled;
        emit volumeEnabledChanged();
        applyAll();
    }
}

bool TrackSettingsModel::sendMidiClock() const
{
    return m_sendMidiClock;
}

void TrackSettingsModel::setSendMidiClock(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling MIDI clock: " << static_cast<int>(enabled);

    if (m_sendMidiClock != enabled) {
        m_sendMidiClock = enabled;
        emit sendMidiClockChanged();
        applyAll();
    }
}

bool TrackSettingsModel::sendTransport() const
{
    return m_sendTransport;
}

void TrackSettingsModel::setSendTransport(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling transport: " << static_cast<int>(enabled);

    if (m_sendTransport != enabled) {
        m_sendTransport = enabled;
        emit sendTransportChanged();
        applyAll();
    }
}

int TrackSettingsModel::delay() const
{
    return m_delay;
}

void TrackSettingsModel::setDelay(int delay)
{
    juzzlin::L(TAG).debug() << "Setting delay to " << delay;

    if (m_delay != delay) {
        m_delay = delay;
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
    return m_velocityJitter;
}

void TrackSettingsModel::setVelocityJitter(int velocityJitter)
{
    juzzlin::L(TAG).debug() << "Setting velocty jitter to " << velocityJitter;

    if (m_velocityJitter != velocityJitter) {
        m_velocityJitter = velocityJitter;
        emit velocityJitterChanged();
    }
}

int TrackSettingsModel::autoNoteOffOffset() const
{
    return m_autoNoteOffOffset;
}

void TrackSettingsModel::setAutoNoteOffOffset(int autoNoteOffOffset)
{
    juzzlin::L(TAG).debug() << "Setting auto note-off offset to " << autoNoteOffOffset;

    if (m_autoNoteOffOffset != autoNoteOffOffset) {
        m_autoNoteOffOffset = autoNoteOffOffset;
        emit autoNoteOffOffsetChanged();
    }
}

bool TrackSettingsModel::autoNoteOffOffsetEnabled() const
{
    return m_autoNoteOffOffsetEnabled;
}

void TrackSettingsModel::setAutoNoteOffOffsetEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling auto note-off offset: " << static_cast<int>(enabled);

    if (m_autoNoteOffOffsetEnabled != enabled) {
        m_autoNoteOffOffsetEnabled = enabled;
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
