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

#include "midi_cc.hpp"

#include <map>

namespace noteahead {

QString midiCcEnumToString(MidiCc midiCc)
{
    static const std::map<MidiCc, QString> enumToStringMap = {
        { MidiCc::BankSelectMSB, "BankSelectMSB" },
        { MidiCc::ModulationWheelMSB, "ModulationWheelMSB" },
        { MidiCc::BreathControllerMSB, "BreathControllerMSB" },
        { MidiCc::FootControllerMSB, "FootControllerMSB" },
        { MidiCc::PortamentoTimeMSB, "PortamentoTimeMSB" },
        { MidiCc::DataEntryMSB, "DataEntryMSB" },
        { MidiCc::ChannelVolumeMSB, "ChannelVolumeMSB" },
        { MidiCc::BalanceMSB, "BalanceMSB" },
        { MidiCc::PanMSB, "PanMSB" },
        { MidiCc::ExpressionControllerMSB, "ExpressionControllerMSB" },
        { MidiCc::EffectControl1MSB, "EffectControl1MSB" },
        { MidiCc::EffectControl2MSB, "EffectControl2MSB" },
        { MidiCc::GeneralPurpose1MSB, "GeneralPurpose1MSB" },
        { MidiCc::GeneralPurpose2MSB, "GeneralPurpose2MSB" },
        { MidiCc::GeneralPurpose3MSB, "GeneralPurpose3MSB" },
        { MidiCc::GeneralPurpose4MSB, "GeneralPurpose4MSB" },
        { MidiCc::BankSelectLSB, "BankSelectLSB" },
        { MidiCc::ModulationWheelLSB, "ModulationWheelLSB" },
        { MidiCc::BreathControllerLSB, "BreathControllerLSB" },
        { MidiCc::FootControllerLSB, "FootControllerLSB" },
        { MidiCc::PortamentoTimeLSB, "PortamentoTimeLSB" },
        { MidiCc::DataEntryLSB, "DataEntryLSB" },
        { MidiCc::ChannelVolumeLSB, "ChannelVolumeLSB" },
        { MidiCc::BalanceLSB, "BalanceLSB" },
        { MidiCc::PanLSB, "PanLSB" },
        { MidiCc::ExpressionControllerLSB, "ExpressionControllerLSB" },
        { MidiCc::EffectControl1LSB, "EffectControl1LSB" },
        { MidiCc::EffectControl2LSB, "EffectControl2LSB" },
        { MidiCc::GeneralPurpose1LSB, "GeneralPurpose1LSB" },
        { MidiCc::GeneralPurpose2LSB, "GeneralPurpose2LSB" },
        { MidiCc::GeneralPurpose3LSB, "GeneralPurpose3LSB" },
        { MidiCc::GeneralPurpose4LSB, "GeneralPurpose4LSB" },
        { MidiCc::SustainPedal, "SustainPedal" },
        { MidiCc::Portamento, "Portamento" },
        { MidiCc::Sostenuto, "Sostenuto" },
        { MidiCc::SoftPedal, "SoftPedal" },
        { MidiCc::LegatoFootswitch, "LegatoFootswitch" },
        { MidiCc::Hold2, "Hold2" },
        { MidiCc::SoundController1, "SoundController1" },
        { MidiCc::SoundController2, "SoundController2" },
        { MidiCc::SoundController3, "SoundController3" },
        { MidiCc::SoundController4, "SoundController4" },
        { MidiCc::SoundController5, "SoundController5" },
        { MidiCc::SoundController6, "SoundController6" },
        { MidiCc::SoundController7, "SoundController7" },
        { MidiCc::SoundController8, "SoundController8" },
        { MidiCc::SoundController9, "SoundController9" },
        { MidiCc::SoundController10, "SoundController10" },
        { MidiCc::GeneralPurpose5, "GeneralPurpose5" },
        { MidiCc::GeneralPurpose6, "GeneralPurpose6" },
        { MidiCc::GeneralPurpose7, "GeneralPurpose7" },
        { MidiCc::GeneralPurpose8, "GeneralPurpose8" },
        { MidiCc::PortamentoControl, "PortamentoControl" },
        { MidiCc::Effects1Depth, "Effects1Depth" },
        { MidiCc::Effects2Depth, "Effects2Depth" },
        { MidiCc::Effects3Depth, "Effects3Depth" },
        { MidiCc::Effects4Depth, "Effects4Depth" },
        { MidiCc::Effects5Depth, "Effects5Depth" },
        { MidiCc::DataIncrement, "DataIncrement" },
        { MidiCc::DataDecrement, "DataDecrement" },
        { MidiCc::NRPNLSB, "NRPNLSB" },
        { MidiCc::NRPNMSB, "NRPNMSB" },
        { MidiCc::RPNLSB, "RPNLSB" },
        { MidiCc::RPNMSB, "RPNMSB" },
        { MidiCc::AllSoundOff, "AllSoundOff" },
        { MidiCc::ResetAllControllers, "ResetAllControllers" },
        { MidiCc::LocalControl, "LocalControl" },
        { MidiCc::AllNotesOff, "AllNotesOff" },
        { MidiCc::OmniModeOff, "OmniModeOff" },
        { MidiCc::OmniModeOn, "OmniModeOn" },
        { MidiCc::MonoModeOn, "MonoModeOn" },
        { MidiCc::PolyModeOn, "PolyModeOn" }
    };
    if (enumToStringMap.contains(midiCc)) {
        return enumToStringMap.at(midiCc);
    }
    return "Undefined";
}

} // namespace noteahead
