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
    if (!m_applyDisabled && !m_portName.isEmpty()) {
        emit applyAllRequested();
    }
}

void TrackSettingsModel::applyMidiCc()
{
    if (!m_applyDisabled && !m_portName.isEmpty()) {
        emit applyMidiCcRequested();
    }
}

void TrackSettingsModel::requestInstrumentData()
{
    emit instrumentDataRequested();
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
    return m_channel;
}

void TrackSettingsModel::setChannel(quint8 channel)
{
    juzzlin::L(TAG).debug() << "Setting channel to " << static_cast<int>(channel);

    if (m_channel != channel) {
        m_channel = channel;
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
    return m_bankEnabled;
}

void TrackSettingsModel::setBankEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling bank: " << static_cast<int>(enabled);

    if (m_bankEnabled != enabled) {
        m_bankEnabled = enabled;
        emit bankEnabledChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::bankLsb() const
{
    return m_bankLsb;
}

void TrackSettingsModel::setBankLsb(quint8 lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::bankMsb() const
{
    return m_bankMsb;
}

void TrackSettingsModel::setBankMsb(quint8 msb)
{
    juzzlin::L(TAG).debug() << "Setting bank MSB to " << static_cast<int>(msb);

    if (m_bankMsb != msb) {
        m_bankMsb = msb;
        emit bankMsbChanged();
        applyAll();
    }
}

bool TrackSettingsModel::bankByteOrderSwapped() const
{
    return m_bankByteOrderSwapped;
}

void TrackSettingsModel::setBankByteOrderSwapped(bool swapped)
{
    juzzlin::L(TAG).debug() << "Enabling swapped bank byte order: " << static_cast<int>(swapped);

    if (m_bankByteOrderSwapped != swapped) {
        m_bankByteOrderSwapped = swapped;
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
    setCutoffEnabled(instrument.settings().predefinedMidiCcSettings.cutoff.has_value());
    if (cutoffEnabled()) {
        setCutoff(*instrument.settings().predefinedMidiCcSettings.cutoff);
    }
    setPanEnabled(instrument.settings().predefinedMidiCcSettings.pan.has_value());
    if (panEnabled()) {
        setPan(*instrument.settings().predefinedMidiCcSettings.pan);
    }
    setVolumeEnabled(instrument.settings().predefinedMidiCcSettings.volume.has_value());
    if (volumeEnabled()) {
        setVolume(*instrument.settings().predefinedMidiCcSettings.volume);
    }
    setSendMidiClock(instrument.settings().sendMidiClock.has_value() && *instrument.settings().sendMidiClock);
    setSendTransport(instrument.settings().sendTransport.has_value() && *instrument.settings().sendTransport);

    setDelay(static_cast<int>(instrument.settings().delay.count()));
    setTranspose(instrument.settings().transpose);

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

    m_portName = {};
    m_channel = 0;
    m_patchEnabled = false;
    m_patch = 0;
    m_bankEnabled = false;
    m_bankLsb = 0;
    m_bankMsb = 0;
    m_bankByteOrderSwapped = false;
    m_cutoffEnabled = false;
    m_cutoff = m_defaultCutoff;
    m_panEnabled = false;
    m_pan = m_defaultPan;
    m_volumeEnabled = false;
    m_volume = m_defaultVolume;
    m_sendMidiClock = false;
    m_sendTransport = false;
    m_delay = 0;
    m_transpose = 0;

    setMidiCcSettings({});

    emit instrumentDataReceived();

    popApplyDisabled();
}

TrackSettingsModel::InstrumentU TrackSettingsModel::toInstrument() const
{
    auto instrument = std::make_unique<Instrument>(m_portName);

    auto address = instrument->midiAddress();
    address.setChannel(m_channel);
    instrument->setMidiAddress(address);

    auto settings = instrument->settings();
    if (m_patchEnabled) {
        settings.patch = m_patch;
    }
    if (m_bankEnabled) {
        settings.bank = {
            m_bankLsb,
            m_bankMsb,
            m_bankByteOrderSwapped
        };
    }
    if (m_cutoffEnabled) {
        settings.predefinedMidiCcSettings.cutoff = m_cutoff;
    }
    if (m_panEnabled) {
        settings.predefinedMidiCcSettings.pan = m_pan;
    }
    if (m_volumeEnabled) {
        settings.predefinedMidiCcSettings.volume = m_volume;
    }
    settings.sendMidiClock = m_sendMidiClock;
    settings.sendTransport = m_sendTransport;
    settings.delay = std::chrono::milliseconds { m_delay };
    settings.transpose = m_transpose;
    settings.midiCcSettings = midiCcSettings();
    instrument->setSettings(settings);

    return instrument;
}

QString TrackSettingsModel::portName() const
{
    return m_portName;
}

void TrackSettingsModel::setPortName(const QString & name)
{
    juzzlin::L(TAG).debug() << "Setting port name to " << std::quoted(name.toStdString());

    if (m_portName != name) {
        m_portName = name;
        emit portNameChanged();
        applyAll();
    }
}

bool TrackSettingsModel::patchEnabled() const
{
    return m_patchEnabled;
}

void TrackSettingsModel::setPatchEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling patch: " << static_cast<int>(enabled);

    if (m_patchEnabled != enabled) {
        m_patchEnabled = enabled;
        emit patchEnabledChanged();
        applyAll();
    }
}

quint8 TrackSettingsModel::patch() const
{
    return m_patch;
}

void TrackSettingsModel::setPatch(quint8 patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_patch != patch) {
        m_patch = patch;
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
    return m_transpose;
}

void TrackSettingsModel::setTranspose(int transpose)
{
    juzzlin::L(TAG).debug() << "Setting transposition to " << transpose;

    if (m_transpose != transpose) {
        m_transpose = transpose;
        emit transposeChanged();
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
