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

#include "event_selection_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/instrument_settings.hpp"

namespace noteahead {

static const auto TAG = "EventSelectionModel";

EventSelectionModel::EventSelectionModel(QObject * parent)
  : QObject { parent }
{
}

void EventSelectionModel::requestData()
{
    emit dataRequested();
}

void EventSelectionModel::save()
{
    emit saveRequested();
}

uint8_t EventSelectionModel::cutoff() const
{
    return m_cutoff;
}

void EventSelectionModel::setCutoff(uint8_t cutoff)
{
    juzzlin::L(TAG).debug() << "Setting cutoff to " << static_cast<int>(cutoff);

    if (m_cutoff != cutoff) {
        m_cutoff = cutoff;
        emit cutoffChanged();
    }
}

bool EventSelectionModel::cutoffEnabled() const
{
    return m_cutoffEnabled;
}

void EventSelectionModel::setCutoffEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling cutoff: " << static_cast<int>(enabled);

    if (m_cutoffEnabled != enabled) {
        m_cutoffEnabled = enabled;
        emit cutoffEnabledChanged();
    }
}

bool EventSelectionModel::bankEnabled() const
{
    return m_bankEnabled;
}

void EventSelectionModel::setBankEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling bank: " << static_cast<int>(enabled);

    if (m_bankEnabled != enabled) {
        m_bankEnabled = enabled;
        emit bankEnabledChanged();
    }
}

uint8_t EventSelectionModel::bankLsb() const
{
    return m_bankLsb;
}

void EventSelectionModel::setBankLsb(uint8_t lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
    }
}

uint8_t EventSelectionModel::bankMsb() const
{
    return m_bankMsb;
}

void EventSelectionModel::setBankMsb(uint8_t msb)
{
    juzzlin::L(TAG).debug() << "Setting bank MSB to " << static_cast<int>(msb);

    if (m_bankMsb != msb) {
        m_bankMsb = msb;
        emit bankMsbChanged();
    }
}

bool EventSelectionModel::bankByteOrderSwapped() const
{
    return m_bankByteOrderSwapped;
}

void EventSelectionModel::setBankByteOrderSwapped(bool swapped)
{
    juzzlin::L(TAG).debug() << "Enabling swapped bank byte order: " << static_cast<int>(swapped);

    if (m_bankByteOrderSwapped != swapped) {
        m_bankByteOrderSwapped = swapped;
        emit bankByteOrderSwappedChanged();
    }
}

void EventSelectionModel::reset()
{
    juzzlin::L(TAG).info() << "Reset";

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

    emit dataReceived();
}

EventSelectionModel::InstrumentSettingsU EventSelectionModel::toInstrumentSettings() const
{
    auto instrumentSettings = std::make_unique<InstrumentSettings>();

    if (m_patchEnabled) {
        instrumentSettings->patch = m_patch;
    }
    if (m_bankEnabled) {
        instrumentSettings->bank = {
            m_bankLsb,
            m_bankMsb,
            m_bankByteOrderSwapped
        };
    }
    if (m_cutoffEnabled) {
        instrumentSettings->predefinedMidiCcSettings.cutoff = m_cutoff;
    }
    if (m_panEnabled) {
        instrumentSettings->predefinedMidiCcSettings.pan = m_pan;
    }
    if (m_volumeEnabled) {
        instrumentSettings->predefinedMidiCcSettings.volume = m_volume;
    }

    return instrumentSettings;
}

void EventSelectionModel::fromInstrumentSettings(const InstrumentSettings & instrumentSettings)
{
    setPatchEnabled(instrumentSettings.patch.has_value());
    if (patchEnabled()) {
        setPatch(*instrumentSettings.patch);
    }

    setBankEnabled(instrumentSettings.bank.has_value());
    if (bankEnabled()) {
        setBankLsb(instrumentSettings.bank->lsb);
        setBankMsb(instrumentSettings.bank->msb);
        setBankByteOrderSwapped(instrumentSettings.bank->byteOrderSwapped);
    }

    setCutoffEnabled(instrumentSettings.predefinedMidiCcSettings.cutoff.has_value());
    if (cutoffEnabled()) {
        setCutoff(*instrumentSettings.predefinedMidiCcSettings.cutoff);
    }

    setPanEnabled(instrumentSettings.predefinedMidiCcSettings.pan.has_value());
    if (panEnabled()) {
        setPan(*instrumentSettings.predefinedMidiCcSettings.pan);
    }

    setVolumeEnabled(instrumentSettings.predefinedMidiCcSettings.volume.has_value());
    if (volumeEnabled()) {
        setVolume(*instrumentSettings.predefinedMidiCcSettings.volume);
    }

    emit dataReceived();
}

bool EventSelectionModel::patchEnabled() const
{
    return m_patchEnabled;
}

void EventSelectionModel::setPatchEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling patch: " << static_cast<int>(enabled);

    if (m_patchEnabled != enabled) {
        m_patchEnabled = enabled;
        emit patchEnabledChanged();
    }
}

uint8_t EventSelectionModel::patch() const
{
    return m_patch;
}

void EventSelectionModel::setPatch(uint8_t patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_patch != patch) {
        m_patch = patch;
        emit patchChanged();
    }
}

uint8_t EventSelectionModel::pan() const
{
    return m_pan;
}

void EventSelectionModel::setPan(uint8_t pan)
{
    juzzlin::L(TAG).debug() << "Setting pan to " << static_cast<int>(pan);

    if (m_pan != pan) {
        m_pan = pan;
        emit panChanged();
    }
}

bool EventSelectionModel::panEnabled() const
{
    return m_panEnabled;
}

void EventSelectionModel::setPanEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling pan: " << static_cast<int>(enabled);

    if (m_panEnabled != enabled) {
        m_panEnabled = enabled;
        emit panEnabledChanged();
    }
}

uint8_t EventSelectionModel::volume() const
{
    return m_volume;
}

void EventSelectionModel::setVolume(uint8_t volume)
{
    juzzlin::L(TAG).debug() << "Setting volume to " << static_cast<int>(volume);

    if (m_volume != volume) {
        m_volume = volume;
        emit volumeChanged();
    }
}

bool EventSelectionModel::volumeEnabled() const
{
    return m_volumeEnabled;
}

void EventSelectionModel::setVolumeEnabled(bool enabled)
{
    juzzlin::L(TAG).debug() << "Enabling volume: " << static_cast<int>(enabled);

    if (m_volumeEnabled != enabled) {
        m_volumeEnabled = enabled;
        emit volumeEnabledChanged();
    }
}

EventSelectionModel::~EventSelectionModel() = default;

} // namespace noteahead
