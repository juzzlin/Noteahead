// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "track_settings_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument.hpp"

namespace cacophony {

static const auto TAG = "TrackSettingsModel";

TrackSettingsModel::TrackSettingsModel(QObject * parent)
  : QObject { parent }
{
}

void TrackSettingsModel::applySettings()
{
    if (!m_isRequestingInstrumentData) {
        emit applySettingsRequested();
    }
}

void TrackSettingsModel::requestInstrumentData()
{
    m_isRequestingInstrumentData = true;

    emit instrumentDataRequested();
}

void TrackSettingsModel::save()
{
    emit saveRequested();
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

    m_isRequestingInstrumentData = false;
}

void TrackSettingsModel::reset()
{
    m_portName = {};
    m_channel = 0;
    m_patchEnabled = false;
    m_patch = 0;
    m_bankEnabled = false;
    m_bankLsb = 0;
    m_bankMsb = 0;
    m_bankByteOrderSwapped = false;

    emit instrumentDataReceived();

    m_isRequestingInstrumentData = false;
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
    if (m_portName != name) {
        m_portName = name;
        emit portNameChanged();
        applySettings();
    }
}

void TrackSettingsModel::setChannel(uint8_t channel)
{
    if (m_channel != channel) {
        m_channel = channel;
        emit channelChanged();
        applySettings();
    }
}

void TrackSettingsModel::setPatchEnabled(bool enabled)
{
    if (m_patchEnabled != enabled) {
        m_patchEnabled = enabled;
        emit patchEnabledChanged();
        applySettings();
    }
}

void TrackSettingsModel::setPatch(uint8_t patch)
{
    if (m_patch != patch) {
        m_patch = patch;
        emit patchChanged();
        applySettings();
    }
}

void TrackSettingsModel::setBankEnabled(bool enabled)
{
    if (m_bankEnabled != enabled) {
        m_bankEnabled = enabled;
        emit bankEnabledChanged();
        applySettings();
    }
}

void TrackSettingsModel::setBankLsb(uint8_t lsb)
{
    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
        applySettings();
    }
}

void TrackSettingsModel::setBankMsb(uint8_t msb)
{
    if (m_bankMsb != msb) {
        m_bankMsb = msb;
        emit bankMsbChanged();
        applySettings();
    }
}

void TrackSettingsModel::setBankByteOrderSwapped(bool swapped)
{
    if (m_bankByteOrderSwapped != swapped) {
        m_bankByteOrderSwapped = swapped;
        emit bankByteOrderSwappedChanged();
        applySettings();
    }
}

TrackSettingsModel::~TrackSettingsModel() = default;

} // namespace cacophony
