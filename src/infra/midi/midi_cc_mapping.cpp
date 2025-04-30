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

#include "midi_cc_mapping.hpp"

#include <map>

namespace noteahead::MidiCcMapping {

QString controllerToString(Controller controller)
{
    static const std::map<Controller, QString> enumToStringMap = {
        { Controller::BankSelectMSB, "BankSelectMSB" },
        { Controller::ModulationWheelMSB, "ModulationWheelMSB" },
        { Controller::BreathControllerMSB, "BreathControllerMSB" },
        { Controller::FootControllerMSB, "FootControllerMSB" },
        { Controller::PortamentoTimeMSB, "PortamentoTimeMSB" },
        { Controller::DataEntryMSB, "DataEntryMSB" },
        { Controller::ChannelVolumeMSB, "ChannelVolumeMSB" },
        { Controller::BalanceMSB, "BalanceMSB" },
        { Controller::PanMSB, "PanMSB" },
        { Controller::ExpressionControllerMSB, "ExpressionControllerMSB" },
        { Controller::EffectControl1MSB, "EffectControl1MSB" },
        { Controller::EffectControl2MSB, "EffectControl2MSB" },
        { Controller::GeneralPurpose1MSB, "GeneralPurpose1MSB" },
        { Controller::GeneralPurpose2MSB, "GeneralPurpose2MSB" },
        { Controller::GeneralPurpose3MSB, "GeneralPurpose3MSB" },
        { Controller::GeneralPurpose4MSB, "GeneralPurpose4MSB" },
        { Controller::BankSelectLSB, "BankSelectLSB" },
        { Controller::ModulationWheelLSB, "ModulationWheelLSB" },
        { Controller::BreathControllerLSB, "BreathControllerLSB" },
        { Controller::FootControllerLSB, "FootControllerLSB" },
        { Controller::PortamentoTimeLSB, "PortamentoTimeLSB" },
        { Controller::DataEntryLSB, "DataEntryLSB" },
        { Controller::ChannelVolumeLSB, "ChannelVolumeLSB" },
        { Controller::BalanceLSB, "BalanceLSB" },
        { Controller::PanLSB, "PanLSB" },
        { Controller::ExpressionControllerLSB, "ExpressionControllerLSB" },
        { Controller::EffectControl1LSB, "EffectControl1LSB" },
        { Controller::EffectControl2LSB, "EffectControl2LSB" },
        { Controller::GeneralPurpose1LSB, "GeneralPurpose1LSB" },
        { Controller::GeneralPurpose2LSB, "GeneralPurpose2LSB" },
        { Controller::GeneralPurpose3LSB, "GeneralPurpose3LSB" },
        { Controller::GeneralPurpose4LSB, "GeneralPurpose4LSB" },
        { Controller::SustainPedal, "SustainPedal" },
        { Controller::Portamento, "Portamento" },
        { Controller::Sostenuto, "Sostenuto" },
        { Controller::SoftPedal, "SoftPedal" },
        { Controller::LegatoFootswitch, "LegatoFootswitch" },
        { Controller::Hold2, "Hold2" },
        { Controller::SoundController1, "SoundController1" },
        { Controller::SoundController2, "SoundController2" },
        { Controller::SoundController3, "SoundController3" },
        { Controller::SoundController4, "SoundController4" },
        { Controller::SoundController5, "SoundController5" },
        { Controller::SoundController6, "SoundController6" },
        { Controller::SoundController7, "SoundController7" },
        { Controller::SoundController8, "SoundController8" },
        { Controller::SoundController9, "SoundController9" },
        { Controller::SoundController10, "SoundController10" },
        { Controller::GeneralPurpose5, "GeneralPurpose5" },
        { Controller::GeneralPurpose6, "GeneralPurpose6" },
        { Controller::GeneralPurpose7, "GeneralPurpose7" },
        { Controller::GeneralPurpose8, "GeneralPurpose8" },
        { Controller::PortamentoControl, "PortamentoControl" },
        { Controller::Effects1Depth, "Effects1Depth" },
        { Controller::Effects2Depth, "Effects2Depth" },
        { Controller::Effects3Depth, "Effects3Depth" },
        { Controller::Effects4Depth, "Effects4Depth" },
        { Controller::Effects5Depth, "Effects5Depth" },
        { Controller::DataIncrement, "DataIncrement" },
        { Controller::DataDecrement, "DataDecrement" },
        { Controller::NRPNLSB, "NRPNLSB" },
        { Controller::NRPNMSB, "NRPNMSB" },
        { Controller::RPNLSB, "RPNLSB" },
        { Controller::RPNMSB, "RPNMSB" },
        { Controller::AllSoundOff, "AllSoundOff" },
        { Controller::ResetAllControllers, "ResetAllControllers" },
        { Controller::LocalControl, "LocalControl" },
        { Controller::AllNotesOff, "AllNotesOff" },
        { Controller::OmniModeOff, "OmniModeOff" },
        { Controller::OmniModeOn, "OmniModeOn" },
        { Controller::MonoModeOn, "MonoModeOn" },
        { Controller::PolyModeOn, "PolyModeOn" }
    };
    if (enumToStringMap.contains(controller)) {
        return enumToStringMap.at(controller);
    }
    return "Undefined";
}

} // namespace noteahead::MidiCcMapping
