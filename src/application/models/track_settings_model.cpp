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

void TrackSettingsModel::requestInstrumentData()
{
    emit instrumentDataRequested();
}

void TrackSettingsModel::requestTestSound(uint8_t velocity)
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

size_t TrackSettingsModel::trackIndex() const
{
    return m_trackIndex;
}

uint8_t TrackSettingsModel::channel() const
{
    return m_channel;
}

void TrackSettingsModel::setChannel(uint8_t channel)
{
    juzzlin::L(TAG).debug() << "Setting channel to " << static_cast<int>(channel);

    if (m_channel != channel) {
        m_channel = channel;
        emit channelChanged();
        applyAll();
    }
}

uint8_t TrackSettingsModel::cutoff() const
{
    return m_cutoff;
}

void TrackSettingsModel::setCutoff(uint8_t cutoff)
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

uint8_t TrackSettingsModel::bankLsb() const
{
    return m_bankLsb;
}

void TrackSettingsModel::setBankLsb(uint8_t lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
        applyAll();
    }
}

uint8_t TrackSettingsModel::bankMsb() const
{
    return m_bankMsb;
}

void TrackSettingsModel::setBankMsb(uint8_t msb)
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

void TrackSettingsModel::setTrackIndex(size_t trackIndex)
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
    m_instrumentPortName = instrument.device.portName;
    setAvailableMidiPorts(m_availableMidiPorts); // Update the list with instrument port name

    setPortName(instrument.device.portName);
    setChannel(instrument.device.channel);
    setPatchEnabled(instrument.settings.patch.has_value());
    if (patchEnabled()) {
        setPatch(*instrument.settings.patch);
    }
    setBankEnabled(instrument.settings.bank.has_value());
    if (bankEnabled()) {
        setBankLsb(instrument.settings.bank->lsb);
        setBankMsb(instrument.settings.bank->msb);
        setBankByteOrderSwapped(instrument.settings.bank->byteOrderSwapped);
    }
    setCutoffEnabled(instrument.settings.cutoff.has_value());
    if (cutoffEnabled()) {
        setCutoff(*instrument.settings.cutoff);
    }
    setPanEnabled(instrument.settings.pan.has_value());
    if (panEnabled()) {
        setPan(*instrument.settings.pan);
    }
    setVolumeEnabled(instrument.settings.volume.has_value());
    if (volumeEnabled()) {
        setVolume(*instrument.settings.volume);
    }
    setMidiCcSettings(instrument.settings.midiCcSettings);

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

    emit instrumentDataReceived();

    popApplyDisabled();
}

TrackSettingsModel::InstrumentU TrackSettingsModel::toInstrument() const
{
    auto instrument = std::make_unique<Instrument>(m_portName);
    instrument->device.channel = m_channel;
    if (m_patchEnabled) {
        instrument->settings.patch = m_patch;
    }
    if (m_bankEnabled) {
        instrument->settings.bank = {
            m_bankLsb,
            m_bankMsb,
            m_bankByteOrderSwapped
        };
    }
    if (m_cutoffEnabled) {
        instrument->settings.cutoff = m_cutoff;
    }
    if (m_panEnabled) {
        instrument->settings.pan = m_pan;
    }
    if (m_volumeEnabled) {
        instrument->settings.volume = m_volume;
    }
    instrument->settings.midiCcSettings = midiCcSettings();
    return instrument;
}

QString TrackSettingsModel::portName() const
{
    return m_portName;
}

void TrackSettingsModel::setPortName(const QString & name)
{
    juzzlin::L(TAG).debug() << "Setting port name to '" << name.toStdString() << "'";

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

uint8_t TrackSettingsModel::patch() const
{
    return m_patch;
}

void TrackSettingsModel::setPatch(uint8_t patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_patch != patch) {
        m_patch = patch;
        emit patchChanged();
        applyAll();
    }
}

uint8_t TrackSettingsModel::pan() const
{
    return m_pan;
}

void TrackSettingsModel::setPan(uint8_t pan)
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

uint8_t TrackSettingsModel::volume() const
{
    return m_volume;
}

void TrackSettingsModel::setVolume(uint8_t volume)
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
