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
  : QObject { parent }
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
    m_applyDisabled = true;

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

// Getters
uint32_t TrackSettingsModel::trackIndex() const
{
    return m_trackIndex;
}

QString TrackSettingsModel::portName() const
{
    return m_portName;
}

uint8_t TrackSettingsModel::channel() const
{
    return m_channel;
}

bool TrackSettingsModel::patchEnabled() const
{
    return m_patchEnabled;
}

uint8_t TrackSettingsModel::patch() const
{
    return m_patch;
}

bool TrackSettingsModel::bankEnabled() const
{
    return m_bankEnabled;
}

uint8_t TrackSettingsModel::bankLsb() const
{
    return m_bankLsb;
}

uint8_t TrackSettingsModel::bankMsb() const
{
    return m_bankMsb;
}

bool TrackSettingsModel::bankByteOrderSwapped() const
{
    return m_bankByteOrderSwapped;
}

// Setters
void TrackSettingsModel::setTrackIndex(uint32_t trackIndex)
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
    m_instrumentPortName = instrument.portName;
    setAvailableMidiPorts(m_availableMidiPorts); // Update the list with instrument port name

    setPortName(instrument.portName);
    setChannel(instrument.channel);
    setPatchEnabled(instrument.patch.has_value());
    if (instrument.patch.has_value()) {
        setPatch(*instrument.patch);
    }
    setBankEnabled(instrument.bank.has_value());
    if (instrument.bank.has_value()) {
        setBankLsb(instrument.bank->lsb);
        setBankMsb(instrument.bank->msb);
        setBankByteOrderSwapped(instrument.bank->byteOrderSwapped);
    }

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

    emit instrumentDataReceived();

    popApplyDisabled();
}

TrackSettingsModel::InstrumentU TrackSettingsModel::toInstrument() const
{
    auto instrument = std::make_unique<Instrument>(m_portName);
    instrument->channel = m_channel;
    if (m_patchEnabled) {
        instrument->patch = m_patch;
    }
    if (m_bankEnabled) {
        instrument->bank = {
            m_bankLsb,
            m_bankMsb,
            m_bankByteOrderSwapped
        };
    }
    return instrument;
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

void TrackSettingsModel::setChannel(uint8_t channel)
{
    juzzlin::L(TAG).debug() << "Setting channel to " << static_cast<int>(channel);

    if (m_channel != channel) {
        m_channel = channel;
        emit channelChanged();
        applyAll();
    }
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

void TrackSettingsModel::setPatch(uint8_t patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_patch != patch) {
        m_patch = patch;
        emit patchChanged();
        applyAll();
    }
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

void TrackSettingsModel::setBankLsb(uint8_t lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
        applyAll();
    }
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

void TrackSettingsModel::setBankByteOrderSwapped(bool swapped)
{
    juzzlin::L(TAG).debug() << "Enabling swapped bank byte order: " << static_cast<int>(swapped);

    if (m_bankByteOrderSwapped != swapped) {
        m_bankByteOrderSwapped = swapped;
        emit bankByteOrderSwappedChanged();
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
