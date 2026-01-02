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

quint8 EventSelectionModel::bankLsb() const
{
    return m_bankLsb;
}

void EventSelectionModel::setBankLsb(quint8 lsb)
{
    juzzlin::L(TAG).debug() << "Setting bank LSB to " << static_cast<int>(lsb);

    if (m_bankLsb != lsb) {
        m_bankLsb = lsb;
        emit bankLsbChanged();
    }
}

quint8 EventSelectionModel::bankMsb() const
{
    return m_bankMsb;
}

void EventSelectionModel::setBankMsb(quint8 msb)
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

quint8 EventSelectionModel::patch() const
{
    return m_patch;
}

void EventSelectionModel::setPatch(quint8 patch)
{
    juzzlin::L(TAG).debug() << "Setting patch to " << static_cast<int>(patch);

    if (m_patch != patch) {
        m_patch = patch;
        emit patchChanged();
    }
}

EventSelectionModel::~EventSelectionModel() = default;

} // namespace noteahead
